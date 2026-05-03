#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include <app/ConfigPortal.h>
#include <app/Controller.h>
#include <devialet/DevialetClient.h>

#if TARGET_M5DIAL
#include <platforms/M5DialHardware.h>
platforms::m5dial::M5DialHardware board;
#else
#error "No hardware target selected"
#endif

#ifndef DEVIALET_HOST
#define DEVIALET_HOST ""
#endif

#ifndef DEVIALET_PORT
#define DEVIALET_PORT 80
#endif

#ifndef DEVIALET_PATH
#define DEVIALET_PATH "/ipcontrol/v1"
#endif

devialet::Client devialetClient;
String serialCommandBuffer;
app::ConfigPortal configPortal;
app::RuntimeConfig runtimeConfig;
app::Controller controller(board, devialetClient);
uint32_t lastDiscoveryAttemptMs = 0;

static void configureDevialetEndpoint(bool force = false);

static void printHelp() {
  Serial.println(F("[serial] commands: help, discover, endpoint, device, status, sources, volume_up, volume_down, volume <0-100>, set_endpoint <host> [port] [path]"));
}

static void printEndpoint() {
  const auto& endpoint = devialetClient.endpoint();
  if (!endpoint.valid()) {
    Serial.println(F("[serial] endpoint: not set"));
    return;
  }
  Serial.printf("[serial] endpoint: %s:%u%s\n", endpoint.host.c_str(), endpoint.port, endpoint.path.c_str());
}

static void printDevice() {
  JsonDocument doc;
  if (!devialetClient.getDevice(doc)) {
    Serial.println(F("[serial] device: failed"));
    return;
  }
  Serial.printf("[serial] device: name=%s model=%s family=%s ipControl=%s firmware=%s leader=%s group=%s\n",
                (const char*)(doc["deviceName"] | ""),
                (const char*)(doc["model"] | ""),
                (const char*)(doc["modelFamily"] | ""),
                (const char*)(doc["ipControlVersion"] | ""),
                (const char*)(doc["firmwareFamily"] | ""),
                (doc["isSystemLeader"] | false) ? "yes" : "no",
                (const char*)(doc["groupId"] | ""));
}

static void printStatus() {
  devialet::Status status;
  if (!devialetClient.getStatus(status)) {
    Serial.println(F("[serial] status: failed"));
    return;
  }
  Serial.printf("[serial] status: volume=%d muted=%s source=%s sourceId=%s playback=%s track=%s artist=%s album=%s\n",
                status.volume,
                status.muted ? "yes" : "no",
                status.sourceName.c_str(),
                status.sourceId.c_str(),
                status.playbackState.c_str(),
                status.track.c_str(),
                status.artist.c_str(),
                status.album.c_str());
}

static void printSources() {
  std::vector<devialet::Source> sources;
  if (!devialetClient.getSources(sources)) {
    Serial.println(F("[serial] sources: failed"));
    return;
  }
  Serial.printf("[serial] sources: count=%u\n", static_cast<unsigned>(sources.size()));
  for (const auto& source : sources) {
    Serial.printf("[serial] source: id=%s name=%s type=%s\n", source.id.c_str(), source.name.c_str(), source.type.c_str());
  }
}

static String tokenAt(const String& line, int index) {
  int current = 0;
  int start = -1;
  for (int i = 0; i <= line.length(); ++i) {
    bool sep = i == line.length() || isspace(static_cast<unsigned char>(line[i]));
    if (!sep && start < 0) start = i;
    if (sep && start >= 0) {
      if (current == index) return line.substring(start, i);
      current++;
      start = -1;
    }
  }
  return "";
}

static void handleSerialCommand(String line) {
  line.trim();
  if (line.isEmpty()) return;
  String cmd = tokenAt(line, 0);
  cmd.toLowerCase();
  Serial.printf("[serial] > %s\n", line.c_str());

  if (cmd == "help" || cmd == "?") {
    printHelp();
  } else if (cmd == "discover") {
    configureDevialetEndpoint(true);
    printEndpoint();
  } else if (cmd == "endpoint") {
    printEndpoint();
  } else if (cmd == "device") {
    printDevice();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "sources") {
    printSources();
  } else if (cmd == "volume_up") {
    Serial.printf("[serial] volume_up: %s\n", devialetClient.volumeUp() ? "ok" : "failed");
    printStatus();
  } else if (cmd == "volume_down") {
    Serial.printf("[serial] volume_down: %s\n", devialetClient.volumeDown() ? "ok" : "failed");
    printStatus();
  } else if (cmd == "volume") {
    int volume = tokenAt(line, 1).toInt();
    Serial.printf("[serial] volume %d: %s\n", volume, devialetClient.setVolume(static_cast<uint8_t>(constrain(volume, 0, 100))) ? "ok" : "failed");
    printStatus();
  } else if (cmd == "set_endpoint") {
    devialet::Endpoint endpoint;
    endpoint.host = tokenAt(line, 1);
    endpoint.port = tokenAt(line, 2).isEmpty() ? 80 : tokenAt(line, 2).toInt();
    endpoint.path = tokenAt(line, 3).isEmpty() ? "/ipcontrol/v1" : tokenAt(line, 3);
    devialetClient.setEndpoint(endpoint);
    printEndpoint();
    printDevice();
  } else {
    Serial.println(F("[serial] unknown command"));
    printHelp();
  }
}

static void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      handleSerialCommand(serialCommandBuffer);
      serialCommandBuffer = "";
    } else if (serialCommandBuffer.length() < 240) {
      serialCommandBuffer += c;
    }
  }
}

static void configureDevialetEndpoint(bool force) {
  if (!force && devialetClient.endpoint().valid()) return;

  uint32_t now = millis();
  if (!force && now - lastDiscoveryAttemptMs < 10000) return;
  lastDiscoveryAttemptMs = now;

  if (!String(DEVIALET_HOST).isEmpty()) {
    devialet::Endpoint endpoint;
    endpoint.host = DEVIALET_HOST;
    endpoint.port = DEVIALET_PORT;
    endpoint.path = DEVIALET_PATH;
    Serial.printf("[devialet] Using compile-time endpoint %s:%u%s\n", endpoint.host.c_str(), endpoint.port, endpoint.path.c_str());
    devialetClient.setEndpoint(endpoint);
    return;
  }

  if (runtimeConfig.hasStaticDevialetEndpoint) {
    Serial.printf("[devialet] Using configured endpoint %s:%u%s\n", runtimeConfig.devialetEndpoint.host.c_str(), runtimeConfig.devialetEndpoint.port, runtimeConfig.devialetEndpoint.path.c_str());
    devialetClient.setEndpoint(runtimeConfig.devialetEndpoint);
    return;
  }

  if (WiFi.status() == WL_CONNECTED && MDNS.begin("devialet-dial")) {
    devialet::Endpoint discovered;
    if (devialetClient.discover(discovered)) {
      Serial.printf("[devialet] Discovered endpoint %s:%u%s\n", discovered.host.c_str(), discovered.port, discovered.path.c_str());
      return;
    }
    Serial.println("[devialet] mDNS discovery found no endpoint");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(800);
  Serial.println("[boot] Devialet Dial starting");
  printHelp();
  controller.begin();

  board.display().showBoot("Hold to reset Wi-Fi");
  uint32_t resetWindowStart = millis();
  while (millis() - resetWindowStart < 3000) {
    auto input = board.input().poll();
    if (input.buttonLongPressed) {
      Serial.println("[config] Reset requested from boot button hold");
      board.display().showBoot("Resetting Wi-Fi");
      configPortal.reset();
      delay(500);
      ESP.restart();
    }
    delay(10);
  }

  board.display().showBoot("Wi-Fi setup");
  bool wifiReady = configPortal.begin(runtimeConfig);
  Serial.printf("[boot] Wi-Fi ready: %s\n", wifiReady ? "yes" : "no");
  board.display().showBoot("Finding Devialet");
  configureDevialetEndpoint(true);
}

void loop() {
  configureDevialetEndpoint(false);
  handleSerialInput();
  controller.loop();
  delay(10);
}

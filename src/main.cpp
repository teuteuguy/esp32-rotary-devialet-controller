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
app::ConfigPortal configPortal;
app::RuntimeConfig runtimeConfig;
app::Controller controller(board, devialetClient);

static void configureDevialetEndpoint() {
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
  configureDevialetEndpoint();
}

void loop() {
  controller.loop();
  delay(10);
}

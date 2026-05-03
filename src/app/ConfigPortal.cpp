#include <app/ConfigPortal.h>

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

namespace app {
namespace {
constexpr const char* kPrefsNamespace = "devdial";
constexpr const char* kPortalSsid = "Devialet Dial Setup";

String readPreference(const char* key, const char* fallback = "") {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  String value = prefs.getString(key, fallback);
  prefs.end();
  return value;
}

void writePreference(const char* key, const String& value) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putString(key, value);
  prefs.end();
}

bool parseEndpoint(RuntimeConfig& config, const String& host, const String& portText, const String& path) {
  String trimmedHost = host;
  trimmedHost.trim();
  if (trimmedHost.isEmpty()) {
    config.hasStaticDevialetEndpoint = false;
    config.devialetEndpoint = {};
    return false;
  }

  int port = portText.toInt();
  if (port <= 0 || port > 65535) port = 80;

  String cleanPath = path;
  cleanPath.trim();
  if (cleanPath.isEmpty()) cleanPath = "/ipcontrol/v1";
  if (!cleanPath.startsWith("/")) cleanPath = "/" + cleanPath;

  config.devialetEndpoint.host = trimmedHost;
  config.devialetEndpoint.port = static_cast<uint16_t>(port);
  config.devialetEndpoint.path = cleanPath;
  config.hasStaticDevialetEndpoint = true;
  return true;
}
} // namespace

bool ConfigPortal::begin(RuntimeConfig& config) {
  WiFi.mode(WIFI_STA);

  String savedHost = readPreference("host");
  String savedPort = readPreference("port", "80");
  String savedPath = readPreference("path", "/ipcontrol/v1");

  char hostBuffer[64];
  char portBuffer[8];
  char pathBuffer[32];
  strlcpy(hostBuffer, savedHost.c_str(), sizeof(hostBuffer));
  strlcpy(portBuffer, savedPort.c_str(), sizeof(portBuffer));
  strlcpy(pathBuffer, savedPath.c_str(), sizeof(pathBuffer));

  WiFiManager wm;
  // No timeout: if Wi-Fi is not configured/reachable, keep the setup AP visible.
  wm.setConnectTimeout(20);
  wm.setConnectRetries(2);
  wm.setTitle("Devialet Dial");

  WiFiManagerParameter devialetHost("devialet_host", "Devialet IP/host (optional)", hostBuffer, sizeof(hostBuffer));
  WiFiManagerParameter devialetPort("devialet_port", "Devialet port", portBuffer, sizeof(portBuffer));
  WiFiManagerParameter devialetPath("devialet_path", "Devialet API path", pathBuffer, sizeof(pathBuffer));
  wm.addParameter(&devialetHost);
  wm.addParameter(&devialetPort);
  wm.addParameter(&devialetPath);

  Serial.println("[config] Starting Wi-Fi manager");
  bool connected = wm.autoConnect(kPortalSsid);
  Serial.printf("[config] Wi-Fi %s, IP=%s\n", connected ? "connected" : "not connected", WiFi.localIP().toString().c_str());

  String host = devialetHost.getValue();
  String port = devialetPort.getValue();
  String path = devialetPath.getValue();

  writePreference("host", host);
  writePreference("port", port.isEmpty() ? "80" : port);
  writePreference("path", path.isEmpty() ? "/ipcontrol/v1" : path);
  bool hasEndpoint = parseEndpoint(config, host, port, path);
  Serial.printf("[config] Static Devialet endpoint %s: %s:%s%s\n", hasEndpoint ? "enabled" : "disabled", host.c_str(), port.c_str(), path.c_str());

  return connected && WiFi.status() == WL_CONNECTED;
}

void ConfigPortal::reset() {
  WiFiManager wm;
  wm.resetSettings();

  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.clear();
  prefs.end();
}

} // namespace app

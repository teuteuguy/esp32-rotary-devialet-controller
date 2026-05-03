#include <app/ConfigPortal.h>

#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace app {
namespace {
constexpr const char* kPrefsNamespace = "devdial";
constexpr const char* kPortalSsid = "DevialetDialSetup";
constexpr const char* kPortalPassword = "devialet";
constexpr byte kDnsPort = 53;

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

String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length());
  for (char c : in) {
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      default: out += c; break;
    }
  }
  return out;
}

String formPage(const String& message = "") {
  String savedHost = htmlEscape(readPreference("host"));
  String savedPort = htmlEscape(readPreference("port", "80"));
  String savedPath = htmlEscape(readPreference("path", "/ipcontrol/v1"));

  String page = F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>Devialet Dial Setup</title>"
                  "<style>body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,sans-serif;margin:24px;max-width:520px}"
                  "input{box-sizing:border-box;width:100%;padding:12px;margin:6px 0 14px;border:1px solid #bbb;border-radius:8px}"
                  "button{padding:12px 18px;border:0;border-radius:8px;background:#111;color:white;font-weight:600}"
                  ".hint{color:#555}.msg{padding:10px;background:#eef;border-radius:8px}</style></head><body>"
                  "<h1>Devialet Dial</h1>");
  if (!message.isEmpty()) page += "<p class='msg'>" + htmlEscape(message) + "</p>";
  page += F("<form method='POST' action='/save'>"
            "<label>Wi-Fi SSID</label><input name='ssid' required autocomplete='off'>"
            "<label>Wi-Fi password</label><input name='pass' type='password' autocomplete='off'>"
            "<h3>Devialet fallback</h3>"
            "<p class='hint'>Leave blank to use mDNS discovery.</p>"
            "<label>Devialet host/IP</label><input name='host' autocomplete='off' value='");
  page += savedHost;
  page += F("'><label>Port</label><input name='port' value='");
  page += savedPort;
  page += F("'><label>API path</label><input name='path' value='");
  page += savedPath;
  page += F("'><button type='submit'>Save & connect</button></form></body></html>");
  return page;
}

bool tryConnect(const String& ssid, const String& pass, uint32_t timeoutMs = 15000) {
  if (ssid.isEmpty()) return false;
  Serial.printf("[config] Connecting to Wi-Fi SSID=%s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }
  Serial.printf("[config] Wi-Fi status=%d IP=%s\n", WiFi.status(), WiFi.localIP().toString().c_str());
  return WiFi.status() == WL_CONNECTED;
}
} // namespace

bool ConfigPortal::begin(RuntimeConfig& config) {
  String savedHost = readPreference("host");
  String savedPort = readPreference("port", "80");
  String savedPath = readPreference("path", "/ipcontrol/v1");
  parseEndpoint(config, savedHost, savedPort, savedPath);

  String savedSsid = readPreference("wifi_ssid");
  String savedPass = readPreference("wifi_pass");
  if (tryConnect(savedSsid, savedPass, 10000)) return true;

  WiFi.disconnect(true, true);
  delay(300);
  WiFi.mode(WIFI_AP_STA);
  bool apStarted = WiFi.softAP(kPortalSsid, kPortalPassword, 6, false, 4);
  IPAddress apIp = WiFi.softAPIP();
  Serial.printf("[config] softAP started=%s SSID=%s password=%s IP=%s\n",
                apStarted ? "yes" : "no", kPortalSsid, kPortalPassword, apIp.toString().c_str());

  DNSServer dns;
  dns.start(kDnsPort, "*", apIp);

  WebServer server(80);
  bool connected = false;

  server.on("/", HTTP_GET, [&]() { server.send(200, "text/html", formPage()); });
  server.on("/generate_204", HTTP_GET, [&]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.on("/fwlink", HTTP_GET, [&]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.on("/save", HTTP_POST, [&]() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String host = server.arg("host");
    String port = server.arg("port");
    String path = server.arg("path");

    writePreference("wifi_ssid", ssid);
    writePreference("wifi_pass", pass);
    writePreference("host", host);
    writePreference("port", port.isEmpty() ? "80" : port);
    writePreference("path", path.isEmpty() ? "/ipcontrol/v1" : path);
    parseEndpoint(config, host, port, path);

    server.send(200, "text/html", formPage("Saved. Trying to connect; you can return to Telegram."));
    delay(500);
    connected = tryConnect(ssid, pass, 20000);
  });
  server.onNotFound([&]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.begin();
  Serial.println("[config] Captive portal HTTP server started");

  while (!connected) {
    dns.processNextRequest();
    server.handleClient();
    delay(5);
  }

  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  return true;
}

void ConfigPortal::reset() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.clear();
  prefs.end();
  WiFi.disconnect(true, true);
}

} // namespace app

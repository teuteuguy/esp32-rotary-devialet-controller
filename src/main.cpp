#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include <app/Controller.h>
#include <devialet/DevialetClient.h>

#if TARGET_M5DIAL
#include <platforms/M5DialHardware.h>
platforms::m5dial::M5DialHardware board;
#else
#error "No hardware target selected"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
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
app::Controller controller(board, devialetClient);

static void connectWifi() {
  if (String(WIFI_SSID).isEmpty()) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }
}

static void configureDevialetEndpoint() {
  if (!String(DEVIALET_HOST).isEmpty()) {
    devialetClient.setEndpoint({DEVIALET_HOST, DEVIALET_PORT, DEVIALET_PATH});
    return;
  }

  if (WiFi.status() == WL_CONNECTED && MDNS.begin("devialet-dial")) {
    devialet::Endpoint discovered;
    if (devialetClient.discover(discovered)) return;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  controller.begin();
  connectWifi();
  configureDevialetEndpoint();
}

void loop() {
  controller.loop();
  delay(10);
}

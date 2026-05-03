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
  delay(200);
  Serial.println("[boot] Devialet Dial starting");
  controller.begin();
  board.display().showBoot("Setup Wi-Fi AP");
  configPortal.begin(runtimeConfig);
  board.display().showBoot("Finding Devialet");
  configureDevialetEndpoint();
}

void loop() {
  controller.loop();
  delay(10);
}

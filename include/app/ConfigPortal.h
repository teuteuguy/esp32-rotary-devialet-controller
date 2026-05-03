#pragma once

#include <Arduino.h>
#include <devialet/DevialetClient.h>

namespace app {

struct RuntimeConfig {
  devialet::Endpoint devialetEndpoint;
  bool hasStaticDevialetEndpoint = false;
};

class ConfigPortal {
public:
  bool begin(RuntimeConfig& config);
  void reset();
};

} // namespace app

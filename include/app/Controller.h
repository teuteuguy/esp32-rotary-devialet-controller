#pragma once

#include <Arduino.h>
#include <devialet/DevialetClient.h>
#include <hardware/Hardware.h>

namespace app {

class Controller {
public:
  Controller(hardware::Hardware& hardware, devialet::Client& devialet);

  void begin();
  void loop();

private:
  hardware::Hardware& hardware_;
  devialet::Client& devialet_;
  devialet::Status status_;
  uint32_t lastRefreshMs_ = 0;
  uint32_t lastVolumeCommandMs_ = 0;
  bool haveStatus_ = false;
  bool errorDisplayed_ = false;
  uint32_t lastInteractionMs_ = 0;

  static constexpr uint32_t kDisplaySleepAfterMs = 30000;

  void refreshStatus(bool force = false);
  void handleInput(const hardware::InputState& input);
  void drawStatus();
  void wakeDisplay();
  void maybeSleepDisplay();
};

} // namespace app

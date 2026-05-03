#include <app/Controller.h>

namespace app {

Controller::Controller(hardware::Hardware& hardware, devialet::Client& devialet)
    : hardware_(hardware), devialet_(devialet) {}

void Controller::begin() {
  hardware_.begin();
  hardware_.display().showBoot("Devialet Dial");
}

void Controller::loop() {
  handleInput(hardware_.input().poll());
  refreshStatus(false);
}

void Controller::refreshStatus(bool force) {
  uint32_t now = millis();
  if (!force && now - lastRefreshMs_ < 1500) return;
  lastRefreshMs_ = now;

  devialet::Status next;
  if (devialet_.getStatus(next)) {
    status_ = next;
    haveStatus_ = true;
    drawStatus();
  } else if (!haveStatus_) {
    hardware_.display().showError("No Devialet");
  }
}

void Controller::handleInput(const hardware::InputState& input) {
  uint32_t now = millis();

  if (input.encoderDelta != 0 && now - lastVolumeCommandMs_ > 80) {
    if (input.encoderDelta > 0) {
      devialet_.volumeUp();
    } else {
      devialet_.volumeDown();
    }
    lastVolumeCommandMs_ = now;
    refreshStatus(true);
  }

  if (input.buttonPressed && haveStatus_) {
    status_.muted ? devialet_.unmute() : devialet_.mute();
    refreshStatus(true);
  }
}

void Controller::drawStatus() {
  hardware_.display().showStatus(status_.sourceName, status_.volume, status_.playbackState, status_.muted);
}

} // namespace app

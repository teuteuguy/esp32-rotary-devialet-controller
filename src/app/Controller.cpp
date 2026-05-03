#include <app/Controller.h>

namespace app {

Controller::Controller(hardware::Hardware& hardware, devialet::Client& devialet)
    : hardware_(hardware), devialet_(devialet) {}

void Controller::begin() {
  hardware_.begin();
  lastInteractionMs_ = millis();
  hardware_.display().showBoot("Devialet Dial");
}

void Controller::loop() {
  handleInput(hardware_.input().poll());
  refreshStatus(false);
  maybeSleepDisplay();
}

void Controller::refreshStatus(bool force) {
  uint32_t now = millis();
  if (!force && now - lastRefreshMs_ < 1500) return;
  lastRefreshMs_ = now;

  devialet::Status next;
  if (devialet_.getStatus(next)) {
    status_ = next;
    haveStatus_ = true;
    errorDisplayed_ = false;
    if (hardware_.display().isAwake()) drawStatus();
  } else if (!haveStatus_ && !errorDisplayed_ && hardware_.display().isAwake()) {
    errorDisplayed_ = true;
    hardware_.display().showError("No Devialet");
  }
}

void Controller::handleInput(const hardware::InputState& input) {
  uint32_t now = millis();
  bool hasInteraction = input.encoderDelta != 0 || input.buttonPressed || input.buttonLongPressed || input.touched;

  if (hasInteraction) {
    lastInteractionMs_ = now;
    if (!hardware_.display().isAwake()) {
      wakeDisplay();
      return;
    }
  }

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

void Controller::wakeDisplay() {
  hardware_.display().setAwake(true);
  if (haveStatus_) {
    drawStatus();
  } else if (errorDisplayed_) {
    hardware_.display().showError("No Devialet");
  } else {
    hardware_.display().showBoot("Finding Devialet");
  }
}

void Controller::maybeSleepDisplay() {
  if (!hardware_.display().isAwake()) return;
  if (millis() - lastInteractionMs_ < kDisplaySleepAfterMs) return;
  hardware_.display().setAwake(false);
}

} // namespace app

#include <platforms/M5DialHardware.h>

#if TARGET_M5DIAL

#include <M5Dial.h>

namespace platforms::m5dial {

void M5DialDisplay::begin() {
  M5Dial.Display.setBrightness(180);
  M5Dial.Display.setTextDatum(middle_center);
}

void M5DialDisplay::showBoot(const String& message) {
  setAwake(true);
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.drawString(message, 120, 105, &fonts::Font4);
  M5Dial.Display.drawString("booting...", 120, 140, &fonts::Font2);
}

void M5DialDisplay::showStatus(const String& sourceName, int volume, const String& playbackState, bool muted) {
  if (!awake_) return;
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.drawString(sourceName.length() ? sourceName : "Devialet", 120, 55, &fonts::Font2);

  M5Dial.Display.setTextColor(muted ? TFT_RED : TFT_CYAN, TFT_BLACK);
  String volumeText = muted ? "MUTE" : String(volume >= 0 ? volume : 0);
  M5Dial.Display.drawString(volumeText, 120, 120, &fonts::Font7);

  M5Dial.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5Dial.Display.drawString(playbackState, 120, 190, &fonts::Font2);
}

void M5DialDisplay::showError(const String& message) {
  if (!awake_) return;
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString(message, 120, 120, &fonts::Font4);
}

void M5DialDisplay::setAwake(bool awake) {
  if (awake == awake_) return;
  awake_ = awake;
  if (awake_) {
    M5Dial.Display.wakeup();
    M5Dial.Display.setBrightness(brightness_);
  } else {
    M5Dial.Display.sleep();
  }
}

void M5DialInput::begin() {
  lastEncoderPosition_ = M5Dial.Encoder.read();
}

hardware::InputState M5DialInput::poll() {
  M5Dial.update();

  hardware::InputState state;
  int32_t pos = M5Dial.Encoder.read();
  state.encoderDelta = pos - lastEncoderPosition_;
  lastEncoderPosition_ = pos;

  bool pressed = M5Dial.BtnA.isPressed();
  if (pressed && !wasPressed_) pressStartedMs_ = millis();
  if (!pressed && wasPressed_) {
    uint32_t held = millis() - pressStartedMs_;
    if (held > 700) state.buttonLongPressed = true;
    else state.buttonPressed = true;
  }
  wasPressed_ = pressed;

  auto detail = M5Dial.Touch.getDetail();
  if (detail.isPressed()) {
    state.touched = true;
    state.touchX = detail.x;
    state.touchY = detail.y;
  }
  return state;
}

void M5DialHardware::begin() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, true);
  display_.begin();
  input_.begin();
}

} // namespace platforms::m5dial

#endif

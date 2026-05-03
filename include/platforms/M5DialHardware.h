#pragma once

#include <hardware/Hardware.h>

#if TARGET_M5DIAL

namespace platforms::m5dial {

class M5DialDisplay final : public hardware::Display {
public:
  void begin() override;
  void showBoot(const String& message) override;
  void showStatus(const String& sourceName, int volume, const String& playbackState, bool muted) override;
  void showError(const String& message) override;
  void setAwake(bool awake) override;
  bool isAwake() const override { return awake_; }

private:
  bool awake_ = true;
  uint8_t brightness_ = 180;
};

class M5DialInput final : public hardware::Input {
public:
  void begin() override;
  hardware::InputState poll() override;

private:
  int32_t lastEncoderPosition_ = 0;
  uint32_t pressStartedMs_ = 0;
  bool wasPressed_ = false;
};

class M5DialHardware final : public hardware::Hardware {
public:
  void begin() override;
  hardware::Display& display() override { return display_; }
  hardware::Input& input() override { return input_; }

private:
  M5DialDisplay display_;
  M5DialInput input_;
};

} // namespace platforms::m5dial

#endif

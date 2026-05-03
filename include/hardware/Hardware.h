#pragma once

#include <Arduino.h>

namespace hardware {

struct InputState {
  int32_t encoderDelta = 0;
  bool buttonPressed = false;
  bool buttonLongPressed = false;
  bool touched = false;
  int16_t touchX = -1;
  int16_t touchY = -1;
};

class Display {
public:
  virtual ~Display() = default;
  virtual void begin() = 0;
  virtual void showBoot(const String& message) = 0;
  virtual void showStatus(const String& sourceName, int volume, const String& playbackState, bool muted) = 0;
  virtual void showError(const String& message) = 0;
};

class Input {
public:
  virtual ~Input() = default;
  virtual void begin() = 0;
  virtual InputState poll() = 0;
};

class Hardware {
public:
  virtual ~Hardware() = default;
  virtual void begin() = 0;
  virtual Display& display() = 0;
  virtual Input& input() = 0;
};

} // namespace hardware

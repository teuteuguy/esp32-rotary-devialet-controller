#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace devialet {

struct Endpoint {
  String host;
  uint16_t port = 80;
  String path = "/ipcontrol/v1";

  bool valid() const { return host.length() > 0 && port > 0 && path.length() > 0; }
  String baseUrl() const { return String("http://") + host + ":" + String(port) + path; }
};

struct Source {
  String id;
  String name;
  String type;
};

struct Status {
  int volume = -1;
  bool muted = false;
  String sourceId;
  String sourceName;
  String playbackState;
  String artist;
  String album;
  String track;
};

class Client {
public:
  explicit Client(uint32_t timeoutMs = 1200);

  bool discover(Endpoint& out);
  void setEndpoint(const Endpoint& endpoint);
  const Endpoint& endpoint() const;

  bool getStatus(Status& out);
  bool getSources(std::vector<Source>& out);
  bool volumeUp();
  bool volumeDown();
  bool setVolume(uint8_t volume);
  bool playSource(const String& sourceId);
  bool mute();
  bool unmute();

private:
  Endpoint endpoint_;
  uint32_t timeoutMs_;

  bool getJson(const String& relativePath, JsonDocument& doc);
  bool postJson(const String& relativePath, const String& body = "{}");
};

} // namespace devialet

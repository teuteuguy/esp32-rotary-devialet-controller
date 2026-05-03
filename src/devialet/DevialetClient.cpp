#include <devialet/DevialetClient.h>

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

namespace devialet {

Client::Client(uint32_t timeoutMs) : timeoutMs_(timeoutMs) {}

void Client::setEndpoint(const Endpoint& endpoint) { endpoint_ = endpoint; }
void Client::clearEndpoint() { endpoint_ = {}; }
const Endpoint& Client::endpoint() const { return endpoint_; }

bool Client::discover(Endpoint& out) {
  int count = MDNS.queryService("http", "tcp");
  Serial.printf("[devialet] mDNS _http._tcp results: %d\n", count);

  for (int i = 0; i < count; ++i) {
    String serviceHost = MDNS.hostname(i);
    String host = MDNS.IP(i).toString();
    uint16_t port = MDNS.port(i);
    String manufacturer = MDNS.txt(i, "manufacturer");
    String ipControlVersion = MDNS.txt(i, "ipControlVersion");
    String advertisedPath = MDNS.txt(i, "path");

    Serial.printf("[devialet] mDNS[%d] serviceHost=%s ip=%s port=%u manufacturer=%s ipControlVersion=%s path=%s\n",
                  i, serviceHost.c_str(), host.c_str(), port, manufacturer.c_str(),
                  ipControlVersion.c_str(), advertisedPath.c_str());

    if (host.isEmpty() || host == "0.0.0.0") continue;

    Endpoint candidate;
    candidate.host = host;
    candidate.port = port == 0 ? 80 : port;
    candidate.path = (advertisedPath.length() > 1) ? advertisedPath : "/ipcontrol/v1";

    JsonDocument device;
    if (!probe(candidate, &device) && candidate.path != "/ipcontrol/v1") {
      candidate.path = "/ipcontrol/v1";
      probe(candidate, &device);
    }

    if (candidate.valid() && probe(candidate, &device)) {
      JsonDocument volumeProbe;
      JsonDocument sourceProbe;
      bool hasVolume = getJsonAt(candidate, "/systems/current/sources/current/soundControl/volume", volumeProbe) && !volumeProbe["volume"].isNull();
      bool hasSource = getJsonAt(candidate, "/groups/current/sources/current", sourceProbe) &&
                       (!sourceProbe["source"].isNull() || !sourceProbe["playingState"].isNull() || !sourceProbe["muteState"].isNull());

      const char* deviceName = device["deviceName"] | "";
      const char* model = device["model"] | "";
      const char* modelFamily = device["modelFamily"] | "";
      bool leader = device["isSystemLeader"] | false;
      Serial.printf("[devialet] Candidate %s:%u%s name=%s model=%s family=%s leader=%s volume=%s source=%s\n",
                    candidate.host.c_str(), candidate.port, candidate.path.c_str(), deviceName, model, modelFamily,
                    leader ? "yes" : "no", hasVolume ? "yes" : "no", hasSource ? "yes" : "no");

      if (!hasVolume || !hasSource) continue;

      out = candidate;
      endpoint_ = out;
      Serial.printf("[devialet] Selected controllable endpoint %s:%u%s name=%s\n",
                    out.host.c_str(), out.port, out.path.c_str(), deviceName);
      return true;
    }
  }
  return false;
}

bool Client::probe(const Endpoint& candidate, JsonDocument* deviceOut) {
  JsonDocument doc;
  if (!getJsonAt(candidate, "/devices/current", doc)) {
    Serial.printf("[devialet] Probe failed %s/devices/current\n", candidate.baseUrl().c_str());
    return false;
  }

  String firmwareFamily = doc["firmwareFamily"] | "";
  String ipControlVersion = doc["ipControlVersion"] | "";
  String modelFamily = doc["modelFamily"] | "";
  bool isDevialetIpControl = firmwareFamily == "DOS" && ipControlVersion == "1" && modelFamily.length() > 0;

  Serial.printf("[devialet] Probe %s => firmware=%s ipControl=%s modelFamily=%s match=%s\n",
                candidate.baseUrl().c_str(), firmwareFamily.c_str(), ipControlVersion.c_str(),
                modelFamily.c_str(), isDevialetIpControl ? "yes" : "no");

  if (!isDevialetIpControl) return false;
  if (deviceOut) *deviceOut = doc;
  return true;
}

bool Client::getDevice(JsonDocument& out) {
  if (!endpoint_.valid()) return false;
  return getJson("/devices/current", out);
}

bool Client::getStatus(Status& out) {
  if (!endpoint_.valid()) return false;

  JsonDocument vol;
  if (getJson("/systems/current/sources/current/soundControl/volume", vol)) {
    out.volume = vol["volume"] | out.volume;
  } else {
    Serial.println(F("[devialet] status volume query failed; clearing endpoint"));
    clearEndpoint();
    return false;
  }

  JsonDocument src;
  if (!getJson("/groups/current/sources/current", src)) {
    Serial.println(F("[devialet] status source query failed; clearing endpoint"));
    clearEndpoint();
    return false;
  }

  JsonVariant source = src["source"];
  out.sourceId = src["sourceId"] | source["sourceId"] | "";
  out.sourceName = src["sourceName"] | src["name"] | source["name"] | source["type"] | out.sourceId;
  out.playbackState = src["playingState"] | src["playbackState"] | "";

  String muteState = src["muteState"] | "";
  out.muted = (src["muted"] | false) || muteState == "muted";

  JsonVariant metadata = src["metadata"];
  if (!metadata.isNull()) {
    out.artist = metadata["artist"] | "";
    out.album = metadata["album"] | "";
    out.track = metadata["track"] | metadata["title"] | "";
  }
  return true;
}

bool Client::getSources(std::vector<Source>& out) {
  JsonDocument doc;
  if (!getJson("/groups/current/sources", doc)) return false;

  out.clear();
  JsonArray sources = doc["sources"].as<JsonArray>();
  for (JsonObject item : sources) {
    Source source;
    source.id = item["sourceId"] | item["id"] | "";
    source.name = item["sourceName"] | item["name"] | source.id;
    source.type = item["sourceType"] | item["type"] | "";
    if (source.id.length() > 0) out.push_back(source);
  }
  return true;
}

bool Client::volumeUp() { return postJson("/systems/current/sources/current/soundControl/volumeUp"); }
bool Client::volumeDown() { return postJson("/systems/current/sources/current/soundControl/volumeDown"); }

bool Client::setVolume(uint8_t volume) {
  JsonDocument doc;
  doc["volume"] = volume > 100 ? 100 : volume;
  String body;
  serializeJson(doc, body);
  return postJson("/systems/current/sources/current/soundControl/volume", body);
}

bool Client::playSource(const String& sourceId) {
  return postJson(String("/groups/current/sources/") + sourceId + "/playback/play");
}

bool Client::mute() { return postJson("/groups/current/sources/current/playback/mute"); }
bool Client::unmute() { return postJson("/groups/current/sources/current/playback/unmute"); }

bool Client::getJson(const String& relativePath, JsonDocument& doc) {
  return getJsonAt(endpoint_, relativePath, doc);
}

bool Client::getJsonAt(const Endpoint& endpoint, const String& relativePath, JsonDocument& doc) {
  if (!endpoint.valid()) return false;
  HTTPClient http;
  http.setTimeout(timeoutMs_);
  String url = endpoint.baseUrl() + relativePath;
  if (!http.begin(url)) return false;
  int code = http.GET();
  if (code != 200) {
    Serial.printf("[devialet] GET %s => HTTP %d\n", url.c_str(), code);
    http.end();
    return false;
  }
  DeserializationError error = deserializeJson(doc, http.getString());
  http.end();
  if (error) Serial.printf("[devialet] JSON parse failed for %s: %s\n", url.c_str(), error.c_str());
  return !error;
}

bool Client::postJson(const String& relativePath, const String& body) {
  HTTPClient http;
  http.setTimeout(timeoutMs_);
  String url = endpoint_.baseUrl() + relativePath;
  if (!http.begin(url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  http.end();
  return code >= 200 && code < 300;
}

} // namespace devialet

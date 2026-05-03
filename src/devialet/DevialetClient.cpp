#include <devialet/DevialetClient.h>

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

namespace devialet {

Client::Client(uint32_t timeoutMs) : timeoutMs_(timeoutMs) {}

void Client::setEndpoint(const Endpoint& endpoint) { endpoint_ = endpoint; }
const Endpoint& Client::endpoint() const { return endpoint_; }

bool Client::discover(Endpoint& out) {
  int count = MDNS.queryService("http", "tcp");
  Serial.printf("[devialet] mDNS _http._tcp results: %d\n", count);

  for (int i = 0; i < count; ++i) {
    String hostname = MDNS.hostname(i);
    String host = MDNS.IP(i).toString();
    uint16_t port = MDNS.port(i);
    String manufacturer = MDNS.txt(i, "manufacturer");
    String ipControlVersion = MDNS.txt(i, "ipControlVersion");
    String path = MDNS.txt(i, "path");

    Serial.printf("[devialet] mDNS[%d] host=%s ip=%s port=%u manufacturer=%s ipControlVersion=%s path=%s\n",
                  i, hostname.c_str(), host.c_str(), port, manufacturer.c_str(),
                  ipControlVersion.c_str(), path.c_str());

    bool hasIpControlTxt = manufacturer.equalsIgnoreCase("Devialet") && ipControlVersion == "1";
    bool looksLikePhantom = hostname.indexOf("Phantom") >= 0;

    // Some Devialet firmware/router combinations advertise only a generic
    // _http._tcp record with TXT path=/, while /ipcontrol/v1 is still present.
    // Prefer proper TXT records, but accept Phantom hostnames as a pragmatic
    // fallback and force the documented IP Control path. Avoid Arch here because
    // it may not represent the playback group we want to control.
    if (!hasIpControlTxt && !looksLikePhantom) continue;

    if (path.isEmpty() || path == "/") path = "/ipcontrol/v1";
    out.host = host;
    out.port = port == 0 ? 80 : port;
    out.path = path;
    endpoint_ = out;
    return true;
  }
  return false;
}

bool Client::getStatus(Status& out) {
  if (!endpoint_.valid()) return false;

  JsonDocument vol;
  if (getJson("/systems/current/sources/current/soundControl/volume", vol)) {
    out.volume = vol["volume"] | out.volume;
  }

  JsonDocument src;
  if (!getJson("/groups/current/sources/current", src)) return false;

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
  HTTPClient http;
  http.setTimeout(timeoutMs_);
  String url = endpoint_.baseUrl() + relativePath;
  if (!http.begin(url)) return false;
  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }
  DeserializationError error = deserializeJson(doc, http.getString());
  http.end();
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

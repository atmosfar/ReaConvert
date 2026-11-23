/*
 * JSON timeline example for ReaConvert -- imports a simple JSON timeline
 * description into REAPER.
 *
 * Public domain / do whatever you want with it.
 */

#include "JsonTimelineParser.h"

#include <cmath>

#include "tiny_json.h"

namespace reaconv {

namespace {

// seconds -> samples
inline long long ToSamples(double seconds, int sampleRate) {
  if (seconds <= 0.0) return 0;
  return (long long)(seconds * sampleRate + 0.5);
}

// Is "p" absolute? (unix root, UNC, or drive letter)
inline bool IsAbsolute(const std::string &p) {
  if (p.empty()) return false;
  if (p[0] == '/' || p[0] == '\\') return true;
  if (p.size() > 2 && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) return true;
  return false;
}

// Relative media paths in the JSON are taken as relative to the JSON file, so
// a demo timeline can ship with its own audio.
std::string ResolveMediaPath(const std::string &baseDir, const std::string &p) {
  if (p.empty() || IsAbsolute(p) || baseDir.empty()) return p;
  std::string s = p;
  for (size_t i = 0; i < s.size(); i++)
    if (s[i] == '\\') s[i] = '/';
  while (s.compare(0, 2, "./") == 0) s.erase(0, 2);
  return baseDir + "/" + s;
}

std::string DirName(const std::string &p) {
  size_t slash = p.find_last_of("/\\");
  if (slash == std::string::npos) return std::string();
  return p.substr(0, slash);
}

// "#rrggbb" -> ColorRGB (unparsed input means "no color")
ColorRGB ParseColor(const tinyjson::Value &v) {
  std::string s = v.asString();
  ColorRGB c;
  if (s.size() != 7 || s[0] != '#') return c;
  int r = 0, g = 0, b = 0;
  if (sscanf(s.c_str() + 1, "%02x%02x%02x", &r, &g, &b) == 3) {
    c = ColorRGB(r, g, b);
  }
  return c;
}

}  // namespace

JsonTimelineParser::JsonTimelineParser()
    : ProjectParser("JSON timeline (ReaConvert example)", "json") {}

bool JsonTimelineParser::Parse(const std::string &filePath, ProjectInfo &project) {
  m_lastError.clear();

  const std::string baseDir = DirName(filePath);

  tinyjson::Value root;
  std::string err;
  if (!tinyjson::parseFile(filePath, root, err)) {
    m_lastError = err;
    return false;
  }
  if (!root.isObject()) {
    m_lastError = "top level value must be an object";
    return false;
  }

  // --- project metadata ---------------------------------------------------
  project.sampleRate = root.member("sampleRate").asInt(48000);
  if (project.sampleRate <= 0) project.sampleRate = 48000;

  project.projectName = root.member("name").asString("JSON timeline");
  project.tempo = root.member("tempo").asNumber(120.0);
  project.timeSigNumerator = 4;
  project.timeSigDenominator = 4;
  const tinyjson::Value &sig = root.member("timeSignature");
  if (sig.isArray() && sig.size() >= 2) {
    project.timeSigNumerator = sig.element(0).asInt(4);
    project.timeSigDenominator = sig.element(1).asInt(4);
  }
  project.frameRate = root.member("frameRate").asNumber(0.0);
  project.dropFrame = false;

  // --- markers ------------------------------------------------------------
  long long maxEnd = 0;
  const tinyjson::Value &markers = root.member("markers");
  int markerIndex = 1;
  for (size_t i = 0; i < markers.size(); i++) {
    const tinyjson::Value &m = markers.element(i);
    if (!m.isObject()) continue;
    Marker mk;
    mk.name = m.member("name").asString("Marker");
    mk.positionSamples = ToSamples(m.member("time").asNumber(0.0), project.sampleRate);
    mk.durationSamples = ToSamples(m.member("length").asNumber(0.0), project.sampleRate);
    mk.index = markerIndex++;
    mk.color = ParseColor(m.member("color"));
    long long end = mk.positionSamples + mk.durationSamples;
    if (end > maxEnd) maxEnd = end;
    project.markers.push_back(mk);
  }

  // --- tracks and clips ---------------------------------------------------
  const tinyjson::Value &tracks = root.member("tracks");
  if (!tracks.isArray() || tracks.size() == 0) {
    m_lastError = "no \"tracks\" array found";
    return false;
  }

  int trackNumber = 1;
  for (size_t i = 0; i < tracks.size(); i++) {
    const tinyjson::Value &t = tracks.element(i);
    if (!t.isObject()) continue;

    Track tr;
    tr.trackNumber = trackNumber++;
    tr.name = t.member("name").asString(("Track " + std::to_string(tr.trackNumber)).c_str());
    tr.color = ParseColor(t.member("color"));
    tr.volume = (float)t.member("volume").asNumber(1.0);
    tr.pan = (float)t.member("pan").asNumber(0.0);
    tr.mute = t.member("mute").asBool(false);
    tr.solo = t.member("solo").asBool(false);
    tr.armed = t.member("recordArmed").asBool(false);
    tr.mainSend = true;

    const tinyjson::Value &clips = t.member("clips");
    for (size_t j = 0; j < clips.size(); j++) {
      const tinyjson::Value &c = clips.element(j);
      if (!c.isObject()) continue;

      MediaClip clip;
      clip.name = c.member("name").asString("clip");
      clip.filePath = ResolveMediaPath(baseDir, c.member("file").asString());
      clip.startPositionSamples = ToSamples(c.member("start").asNumber(0.0), project.sampleRate);
      long long len = ToSamples(c.member("length").asNumber(0.0), project.sampleRate);
      clip.endPositionSamples = clip.startPositionSamples + len;
      clip.sourceOffsetSamples = ToSamples(c.member("offset").asNumber(0.0), project.sampleRate);
      clip.volume = (float)c.member("volume").asNumber(1.0);
      clip.pan = (float)c.member("pan").asNumber(0.0);
      clip.gain = (float)c.member("gain").asNumber(1.0);
      clip.mute = c.member("mute").asBool(false);
      clip.loop = c.member("loop").asBool(true);
      clip.fadeInDuration = c.member("fadeIn").asNumber(0.0);
      clip.fadeOutDuration = c.member("fadeOut").asNumber(0.0);
      clip.color = ParseColor(c.member("color"));

      if (clip.endPositionSamples > maxEnd) maxEnd = clip.endPositionSamples;
      tr.clips.push_back(clip);
    }

    project.tracks.push_back(tr);
  }

  project.durationSamples = maxEnd > 0 ? maxEnd : ToSamples(60.0, project.sampleRate);

  if (project.tracks.empty()) {
    m_lastError = "\"tracks\" array contained no usable tracks";
    return false;
  }
  return true;
}

}  // namespace reaconv

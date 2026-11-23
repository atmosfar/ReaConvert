/*
 * JSON timeline example for ReaConvert -- exports the current REAPER project
 * as a simple JSON timeline description.
 *
 * Public domain / do whatever you want with it.
 */

#include "JsonTimelineWriter.h"

#include <cstdio>

#include "tiny_json.h"

namespace reaconv {

namespace {

// samples -> seconds, formatted without trailing zeros
std::string Num(double v) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%.9f", v);
  std::string s(buf);
  size_t dot = s.find('.');
  if (dot != std::string::npos) {
    size_t last = s.find_last_not_of('0');
    if (last == dot) last = dot - 1;
    s.erase(last + 1);
  }
  if (s.empty() || s == "-") s = "0";
  return s;
}

std::string Samples(long long samples, int sampleRate) {
  if (sampleRate <= 0) sampleRate = 48000;
  return Num((double)samples / (double)sampleRate);
}

std::string Color(const ColorRGB &c) {
  if (!c.hasColor) return "null";
  char buf[16];
  snprintf(buf, sizeof(buf), "\"#%02x%02x%02x\"", c.r & 0xFF, c.g & 0xFF, c.b & 0xFF);
  return std::string(buf);
}

std::string Bool(bool b) { return b ? "true" : "false"; }

}  // namespace

JsonTimelineWriter::JsonTimelineWriter()
    : ProjectWriter("JSON timeline (ReaConvert example)", "json") {}

std::string JsonTimelineWriter::ToString(const ProjectInfo &project) const {
  int sr = project.sampleRate > 0 ? project.sampleRate : 48000;

  tinyjson::Writer w;
  w.openLine("{");
  w.line("\"format\": \"reaconv-timeline\",");
  w.line("\"version\": 1,");
  w.line("\"name\": %s,", tinyjson::quoted(project.projectName).c_str());
  w.line("\"sampleRate\": %d,", sr);
  w.line("\"tempo\": %s,", Num(project.tempo > 0 ? project.tempo : 120.0).c_str());
  w.line("\"timeSignature\": [%d, %d],", project.timeSigNumerator > 0 ? project.timeSigNumerator : 4,
         project.timeSigDenominator > 0 ? project.timeSigDenominator : 4);

  if (!project.markers.empty()) {
    w.openLine("\"markers\": [");
    for (size_t i = 0; i < project.markers.size(); i++) {
      const Marker &m = project.markers[i];
      std::string tail = (i + 1 < project.markers.size()) ? "," : "";
      w.openLine("{");
      w.line("\"name\": %s,", tinyjson::quoted(m.name).c_str());
      w.line("\"time\": %s,", Samples(m.positionSamples, sr).c_str());
      w.line("\"length\": %s,", Samples(m.durationSamples, sr).c_str());
      w.line("\"color\": %s", Color(m.color).c_str());
      w.closeLine("}" + tail);
    }
    w.closeLine("],");
  }

  w.openLine("\"tracks\": [");
  for (size_t i = 0; i < project.tracks.size(); i++) {
    const Track &t = project.tracks[i];
    std::string tail = (i + 1 < project.tracks.size()) ? "," : "";

    w.openLine("{");
    w.line("\"name\": %s,", tinyjson::quoted(t.name).c_str());
    w.line("\"color\": %s,", Color(t.color).c_str());
    w.line("\"volume\": %s,", Num(t.volume).c_str());
    w.line("\"pan\": %s,", Num(t.pan).c_str());
    w.line("\"mute\": %s,", Bool(t.mute).c_str());
    w.line("\"solo\": %s,", Bool(t.solo).c_str());
    w.line("\"recordArmed\": %s,", Bool(t.armed).c_str());

    w.openLine("\"clips\": [");
    for (size_t j = 0; j < t.clips.size(); j++) {
      const MediaClip &c = t.clips[j];
      std::string ctail = (j + 1 < t.clips.size()) ? "," : "";
      w.openLine("{");
      w.line("\"name\": %s,", tinyjson::quoted(c.name).c_str());
      w.line("\"file\": %s,", tinyjson::quoted(c.filePath).c_str());
      w.line("\"start\": %s,", Samples(c.startPositionSamples, sr).c_str());
      w.line("\"length\": %s,", Samples(c.endPositionSamples - c.startPositionSamples, sr).c_str());
      w.line("\"offset\": %s,", Samples(c.sourceOffsetSamples, sr).c_str());
      w.line("\"volume\": %s,", Num(c.volume).c_str());
      w.line("\"pan\": %s,", Num(c.pan).c_str());
      w.line("\"gain\": %s,", Num(c.gain).c_str());
      w.line("\"fadeIn\": %s,", Num(c.fadeInDuration).c_str());
      w.line("\"fadeOut\": %s,", Num(c.fadeOutDuration).c_str());
      w.line("\"loop\": %s,", Bool(c.loop).c_str());
      w.line("\"mute\": %s,", Bool(c.mute).c_str());
      w.line("\"color\": %s", Color(c.color).c_str());
      w.closeLine("}" + ctail);
    }
    w.closeLine("]");
    w.closeLine("}" + tail);
  }
  w.closeLine("]");
  w.closeLine("}");

  return w.str();
}

bool JsonTimelineWriter::Write(const ProjectInfo &project, const std::string &filePath) {
  m_lastError.clear();
  std::string text = ToString(project);

  FILE *fp = fopen(filePath.c_str(), "wb");
  if (!fp) {
    m_lastError = "could not open '" + filePath + "' for writing";
    return false;
  }
  size_t n = fwrite(text.data(), 1, text.size(), fp);
  fclose(fp);
  if (n != text.size()) {
    m_lastError = "short write to '" + filePath + "'";
    return false;
  }
  return true;
}

}  // namespace reaconv

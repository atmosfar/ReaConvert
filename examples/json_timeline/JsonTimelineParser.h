/*
 * JSON timeline example for ReaConvert -- imports a simple JSON timeline
 * description into REAPER.
 *
 * Public domain / do whatever you want with it.
 */

#ifndef JSON_TIMELINE_PARSER_H
#define JSON_TIMELINE_PARSER_H

#include <string>

#include "common/ProjectInfo.h"
#include "common/ProjectParser.h"

namespace reaconv {

// Reads the example schema (see README.md) into a ProjectInfo.
class JsonTimelineParser : public ProjectParser {
public:
  JsonTimelineParser();

  bool Parse(const std::string &filePath, ProjectInfo &project) override;

  // populated when Parse() returns false
  const std::string &LastError() const { return m_lastError; }

private:
  std::string m_lastError;
};

}  // namespace reaconv

#endif  // JSON_TIMELINE_PARSER_H

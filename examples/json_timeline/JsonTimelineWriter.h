/*
 * JSON timeline example for ReaConvert -- exports the current REAPER project
 * as a simple JSON timeline description.
 *
 * Public domain / do whatever you want with it.
 */

#ifndef JSON_TIMELINE_WRITER_H
#define JSON_TIMELINE_WRITER_H

#include <string>

#include "common/ProjectInfo.h"
#include "common/ProjectWriter.h"

namespace reaconv {

// Writes a ProjectInfo out using the example schema (see README.md).
class JsonTimelineWriter : public ProjectWriter {
public:
  JsonTimelineWriter();

  bool Write(const ProjectInfo &project, const std::string &filePath) override;

  // the same document that Write() would put on disk, useful for logging
  std::string ToString(const ProjectInfo &project) const;

  const std::string &LastError() const { return m_lastError; }

private:
  std::string m_lastError;
};

}  // namespace reaconv

#endif  // JSON_TIMELINE_WRITER_H

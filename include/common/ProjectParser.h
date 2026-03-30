/*
 * Copyright (C) 2026 atmosfar
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PROJECT_PARSER_H
#define PROJECT_PARSER_H

#include <string>
#include "ProjectInfo.h"

namespace reaconv {

class ProjectParser {
public:
    ProjectParser(const std::string& desc, const std::string& ext)
        : formatDescription(desc), formatFileExtension(ext) {}
    virtual ~ProjectParser() = default;

    virtual bool Parse(const std::string& filePath, ProjectInfo& project) = 0;

    const std::string& GetFormatDescription() const { return formatDescription; }
    const std::string& GetFormatFileExtension() const { return formatFileExtension; }

private:
    // Brief project format description, eg. "Reaper Project"
    const std::string formatDescription;
    // Project format file extension, eg. "rpp"
    const std::string formatFileExtension;
};

} // namespace reaconv

#endif // PROJECT_PARSER_H

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

#ifndef PROJECT_WRITER_H
#define PROJECT_WRITER_H

#include <string>
#include "ProjectInfo.h"

namespace reaconv {

class ProjectWriter {
public:
    ProjectWriter() = default;
    virtual ~ProjectWriter() = default;

    virtual bool Write(const ProjectInfo& project, const std::string& filePath) = 0;
};

} // namespace reaconv

#endif // PROJECT_WRITER_H

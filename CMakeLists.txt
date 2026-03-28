cmake_minimum_required(VERSION 3.10)
project(ReaConvert VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force macOS architecture when generating on Apple hosts.
if(APPLE)
  if(NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
    # Prefer host processor (Rosetta terminal will report x86_64)
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
    message(STATUS "Apple architecture set from HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif()
endif()

# SWELL configuration for plugins
add_definitions(-DSWELL_PROVIDED_BY_APP)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
    add_compile_options(/wd4267 /wd4838 /wd4244)
elseif(UNIX AND NOT APPLE)
    # Linux
    add_definitions(-D_GNU_SOURCE)
endif()

# ReaConvert library sources
set(REACONVERT_SOURCES
    src/RppParser.cpp
    src/RppWriter.cpp
)

# Add platform-specific WDL sources
if(APPLE)
    list(APPEND REACONVERT_SOURCES
        WDL/WDL/win32_utf8.c
        WDL/WDL/swell/swell-modstub.mm
    )
    # ensure C++ compilation for win32_utf8.c
    set_source_files_properties(WDL/WDL/win32_utf8.c PROPERTIES LANGUAGE CXX)
elseif(WIN32)
    list(APPEND REACONVERT_SOURCES
        WDL/WDL/win32_utf8.c
    )
elseif(UNIX)
    # Linux/Unix
    list(APPEND REACONVERT_SOURCES
        WDL/WDL/win32_utf8.c
        WDL/WDL/swell/swell-modstub-generic.cpp
    )
    # ensure C++ compilation for win32_utf8.c
    set_source_files_properties(WDL/WDL/win32_utf8.c PROPERTIES LANGUAGE CXX)
endif()

# Create static library
add_library(ReaConvert STATIC ${REACONVERT_SOURCES})

# Set target properties
set_target_properties(ReaConvert PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Export include directories for consumers
target_include_directories(ReaConvert PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/reaper_sdk>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/common>
    $<INSTALL_INTERFACE:reaper_sdk>
    $<INSTALL_INTERFACE:WDL/WDL>
)

# Install targets
install(TARGETS ReaConvert
    EXPORT ReaConvertTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Export package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReaConvertConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ReaConvertConfig.cmake
    INSTALL_DESTINATION lib/cmake/ReaConvert
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ReaConvertConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ReaConvertConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ReaConvertConfigVersion.cmake
    DESTINATION lib/cmake/ReaConvert
)

install(EXPORT ReaConvertTargets
    FILE ReaConvertTargets.cmake
    NAMESPACE ReaConvert::
    DESTINATION lib/cmake/ReaConvert
)

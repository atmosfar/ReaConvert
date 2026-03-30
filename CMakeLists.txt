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
        extern/WDL/WDL/win32_utf8.c
        extern/WDL/WDL/swell/swell-modstub.mm
    )
    # ensure C++ compilation for win32_utf8.c
    set_source_files_properties(extern/WDL/WDL/win32_utf8.c PROPERTIES LANGUAGE CXX)
elseif(WIN32)
    list(APPEND REACONVERT_SOURCES
        extern/WDL/WDL/win32_utf8.c
    )
elseif(UNIX)
    # Linux/Unix
    list(APPEND REACONVERT_SOURCES
        extern/WDL/WDL/win32_utf8.c
        extern/WDL/WDL/swell/swell-modstub-generic.cpp
    )
    # ensure C++ compilation for win32_utf8.c
    set_source_files_properties(extern/WDL/WDL/win32_utf8.c PROPERTIES LANGUAGE CXX)
endif()

# Create static library
add_library(ReaConvert STATIC ${REACONVERT_SOURCES})

# REAPER API minimal mode
target_compile_definitions(ReaConvert PUBLIC
    REAPERAPI_MINIMAL
    REAPERAPI_WANT_GetAppVersion
    REAPERAPI_WANT_ShowConsoleMsg
    REAPERAPI_WANT_ShowMessageBox
    REAPERAPI_WANT_CountTracks
    REAPERAPI_WANT_GetTrack
    REAPERAPI_WANT_GetSetMediaTrackInfo_String
    REAPERAPI_WANT_GetMediaTrackInfo_Value
    REAPERAPI_WANT_GetTrackColor
    REAPERAPI_WANT_GetMasterTrack
    REAPERAPI_WANT_GetSetProjectInfo_String
    REAPERAPI_WANT_GetSetProjectInfo
    REAPERAPI_WANT_GetProjectName
    REAPERAPI_WANT_GetProjectPath
    REAPERAPI_WANT_EnumProjects
    REAPERAPI_WANT_GetProjectLength
    REAPERAPI_WANT_GetTrackNumMediaItems
    REAPERAPI_WANT_GetTrackMediaItem
    REAPERAPI_WANT_GetActiveTake
    REAPERAPI_WANT_GetSetMediaItemTakeInfo_String
    REAPERAPI_WANT_GetMediaItemTakeInfo_Value
    REAPERAPI_WANT_GetMediaItemTake_Item
    REAPERAPI_WANT_GetMediaItemInfo_Value
    REAPERAPI_WANT_GetMediaItemTake_Source
    REAPERAPI_WANT_GetMediaSourceFileName
    REAPERAPI_WANT_EnumProjectMarkers3
    REAPERAPI_WANT_GetTrackNumSends
    REAPERAPI_WANT_GetTrackSendInfo_Value
    REAPERAPI_WANT_GetTrackEnvelopeByName
    REAPERAPI_WANT_CountTrackEnvelopes
    REAPERAPI_WANT_GetTrackEnvelope
    REAPERAPI_WANT_GetTakeEnvelopeByName
    REAPERAPI_WANT_CountTakeEnvelopes
    REAPERAPI_WANT_GetTakeEnvelope
    REAPERAPI_WANT_GetEnvelopeName
    REAPERAPI_WANT_GetEnvelopeStateChunk
    REAPERAPI_WANT_CountEnvelopePoints
    REAPERAPI_WANT_GetEnvelopePoint
    REAPERAPI_WANT_GetSetEnvelopeInfo_String
    REAPERAPI_WANT_CountAutomationItems
    REAPERAPI_WANT_CountEnvelopePointsEx
    REAPERAPI_WANT_GetEnvelopePointEx
    REAPERAPI_WANT_ScaleFromEnvelopeMode
    REAPERAPI_WANT_plugin_register
    REAPERAPI_WANT_GetMasterMuteSoloFlags
    REAPERAPI_WANT_TrackFX_GetCount
    REAPERAPI_WANT_TrackFX_GetFXName
    REAPERAPI_WANT_TrackFX_GetParamName
    REAPERAPI_WANT_TrackFX_GetParam
    REAPERAPI_WANT_TrackFX_GetEnabled
    REAPERAPI_WANT_TrackFX_GetNamedConfigParm
    REAPERAPI_WANT_TrackFX_GetNumParams
    REAPERAPI_WANT_GetFXEnvelope
    REAPERAPI_WANT_TakeFX_GetCount
    REAPERAPI_WANT_TakeFX_GetFXName
    REAPERAPI_WANT_TakeFX_GetParamName
    REAPERAPI_WANT_TakeFX_GetParam
    REAPERAPI_WANT_TakeFX_GetEnabled
    REAPERAPI_WANT_TakeFX_GetNamedConfigParm
    REAPERAPI_WANT_TakeFX_GetNumParams
    REAPERAPI_WANT_TakeFX_GetEnvelope
    REAPERAPI_WANT_TrackFX_GetPreset
    REAPERAPI_WANT_TrackFX_GetParamEx
    REAPERAPI_WANT_TakeFX_GetPreset
    REAPERAPI_WANT_TakeFX_GetParamEx
    REAPERAPI_WANT_GetMediaSourceType
    REAPERAPI_WANT_GetItemStateChunk
    REAPERAPI_WANT_GetTrackStateChunk
    REAPERAPI_WANT_ColorFromNative
    REAPERAPI_WANT_ColorToNative
    REAPERAPI_WANT_TimeMap_curFrameRate
    REAPERAPI_WANT_GetNumTakeMarkers
    REAPERAPI_WANT_GetTakeMarker
)

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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/reaper-sdk/sdk>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/WDL/WDL>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/common>
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

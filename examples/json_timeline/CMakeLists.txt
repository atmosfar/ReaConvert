# Example REAPER extension: two-way JSON timeline import/export.
#
#   cmake -S . -B build && cmake --build build
#   -> build/examples/json_timeline/reaper_json_convert-arm64.dylib
#
# Naming and deployment follow the same conventions as the other REAPER
# extensions: file name starts with "reaper_" and carries the target
# architecture in the suffix, and the result is copied into REAPER's
# UserPlugins directory after every build.

cmake_minimum_required(VERSION 3.10)

option(REACONVERT_EXAMPLE_DEPLOY
       "Copy the built example extension into REAPER's UserPlugins" ON)

# PREFIX "reaper_" plus the arch in SUFFIX produce
# reaper_json_convert-arm64.dylib / -x64.dll / -x86_64.so
set(EXAMPLE_TARGET json_convert)

add_library(${EXAMPLE_TARGET} SHARED
    example_extension.cpp
    JsonTimelineParser.cpp
    JsonTimelineWriter.cpp
)

# ReaConvert's PUBLIC include dirs (include/, reaper-sdk/sdk, WDL) come with this
target_link_libraries(${EXAMPLE_TARGET} PRIVATE ReaConvert)

# The extension only touches a handful of REAPER API calls; declaring just those
# keeps the compile fast (same minimal mode the ReaConvert library itself uses).
target_compile_definitions(${EXAMPLE_TARGET} PRIVATE
    SWELL_PROVIDED_BY_APP
    REAPERAPI_MINIMAL
    REAPERAPI_WANT_EnumProjects
    REAPERAPI_WANT_GetProjectPath
    REAPERAPI_WANT_GetSetProjectInfo_String
    REAPERAPI_WANT_ShowMessageBox
    REAPERAPI_WANT_ShowConsoleMsg
)

set_target_properties(${EXAMPLE_TARGET} PROPERTIES
    PREFIX "reaper_"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(${EXAMPLE_TARGET} PRIVATE /wd4244 /wd4267)
endif()

if(APPLE)
    # Pick the arch the way REAPER extensions expect. CMAKE_HOST_SYSTEM_PROCESSOR
    # is used rather than the target one because a Rosetta terminal reports
    # x86_64, which is the arch the compiler will actually build for.
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(EXAMPLE_ARCH universal)
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(EXAMPLE_ARCH arm64)
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(EXAMPLE_ARCH x86_64)
    elseif(CMAKE_OSX_ARCHITECTURES)
        set(EXAMPLE_ARCH ${CMAKE_OSX_ARCHITECTURES})
    else()
        set(EXAMPLE_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()

    set_target_properties(${EXAMPLE_TARGET} PROPERTIES SUFFIX "-${EXAMPLE_ARCH}.dylib")

    # ReaConvert pulls in Objective-C++ SWELL stubs
    find_library(APPKIT_LIBRARY AppKit)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE
        ${APPKIT_LIBRARY}
        ${FOUNDATION_LIBRARY}
        ${COCOA_LIBRARY}
    )
elseif(WIN32)
    set_target_properties(${EXAMPLE_TARGET} PROPERTIES SUFFIX "-x64.dll")
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE comdlg32 shell32 shlwapi)
else()
    # Linux
    set_target_properties(${EXAMPLE_TARGET} PROPERTIES SUFFIX "-x86_64.so")
    find_library(DL_LIBRARY dl)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE ${DL_LIBRARY} pthread)
endif()

# Automated deployment into REAPER's UserPlugins
if(APPLE)
    set(REAPER_USER_PLUGINS "$ENV{HOME}/Library/Application Support/REAPER/UserPlugins")
elseif(WIN32)
    set(REAPER_USER_PLUGINS "$ENV{APPDATA}/REAPER/UserPlugins")
elseif(UNIX)
    set(REAPER_USER_PLUGINS "$ENV{HOME}/.config/REAPER/UserPlugins")
endif()

if(REACONVERT_EXAMPLE_DEPLOY AND REAPER_USER_PLUGINS)
    add_custom_command(TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${REAPER_USER_PLUGINS}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${EXAMPLE_TARGET}>
                "${REAPER_USER_PLUGINS}/$<TARGET_FILE_NAME:${EXAMPLE_TARGET}>"
        COMMENT "Deploying $<TARGET_FILE_NAME:${EXAMPLE_TARGET}> to ${REAPER_USER_PLUGINS}"
        VERBATIM
    )
endif()

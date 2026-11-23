/*
 * ReaConvert example extension: a two-way "JSON timeline" project format.
 *
 * IMPORT   File > Open / Project > Insert  some_timeline.json  ->  REAPER project
 *          (JsonTimelineParser -> ProjectInfo -> RppWriter)
 *
 * EXPORT   Action: "ReaConvert example: export project as JSON timeline"
 *          (RppParser -> ProjectInfo -> JsonTimelineWriter)
 *
 * This file is intentionally small and readable: it demonstrates how little
 * glue ReaConvert needs, it is not a hardened product.
 *
 * Public domain / do whatever you want with it.
 */

#include <cstdio>
#include <cstring>
#include <string>

// include reaper_plugin.h before reaper_plugin_functions.h (see that header's notes)
#include "reaper_plugin.h"

#include "common/ProjectInfo.h"
#include "RppParser.h"
#include "RppWriter.h"
#include "ReaConvert_LoadAPI.h"

#include "JsonTimelineParser.h"
#include "JsonTimelineWriter.h"

static int g_exportCommandId = 0;

// ---------------------------------------------------------------- helpers ---

static bool EqualIgnoreCase(const char *a, const char *b) {
  while (*a && *b) {
    char ca = *a, cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca += 32;
    if (cb >= 'A' && cb <= 'Z') cb += 32;
    if (ca != cb) return false;
    a++;
    b++;
  }
  return *a == 0 && *b == 0;
}

static bool HasJsonExtension(const char *fn) {
  if (!fn) return false;
  size_t l = strlen(fn);
  return l > 5 && EqualIgnoreCase(fn + l - 5, ".json");
}

static std::string StripTrailingSlash(std::string s) {
  while (s.size() > 1 && (s[s.size() - 1] == '/' || s[s.size() - 1] == '\\')) s.erase(s.size() - 1);
  return s;
}

static std::string SanitizeForFilename(std::string s) {
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' ||
        c == '|')
      s[i] = '_';
  }
  if (s.empty()) s = "untitled";
  return s;
}

static int CountClips(const reaconv::ProjectInfo &info) {
  int clips = 0;
  for (size_t i = 0; i < info.tracks.size(); i++) clips += (int)info.tracks[i].clips.size();
  return clips;
}

// ------------------------------------------------------------------ import ---

// Claim a .json file only when it looks like our own format, so that unrelated
// JSON files are left alone.
static bool WantProjectFile(const char *fn) {
  if (!HasJsonExtension(fn)) return false;

  FILE *fp = fopen(fn, "rb");
  if (!fp) return false;
  char buf[256];
  size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
  fclose(fp);
  buf[n] = 0;
  return strstr(buf, "reaconv-timeline") != NULL;
}

static const char *EnumFileExtensions(int i, char **descptr) {
  static char desc[] = "JSON timeline (ReaConvert example)";
  if (i == 0) {
    if (descptr) *descptr = desc;
    return "JSON";
  }
  if (descptr) *descptr = NULL;
  return NULL;
}

static int LoadProject(const char *fn, ProjectStateContext *genstate) {
  reaconv::ProjectInfo info;
  reaconv::JsonTimelineParser parser;

  if (!parser.Parse(fn, info)) {
    std::string msg = "ReaConvert example: could not read this JSON timeline.\n\n";
    msg += fn;
    msg += "\n\n";
    msg += parser.LastError();
    if (ShowMessageBox) ShowMessageBox(msg.c_str(), "ReaConvert example", 0);
    return -1;
  }

  // one call: ProjectInfo in, REAPER project chunk out
  reaconv::RppWriter writer;
  writer.Write(info, genstate);

  if (ShowConsoleMsg) {
    char buf[512];
    // "!SHOW:" logs to the REAPER console without popping the window open
    snprintf(buf, sizeof(buf),
             "!SHOW:ReaConvert example: imported \"%s\" -- %d track(s), %d clip(s), %d marker(s)\n",
             info.projectName.c_str(), (int)info.tracks.size(), CountClips(info), (int)info.markers.size());
    ShowConsoleMsg(buf);
  }
  return 0;
}

static project_import_register_t g_importRegister = {WantProjectFile, EnumFileExtensions, LoadProject};

// ------------------------------------------------------------------ export ---

static void ExportActiveProject() {
  if (!EnumProjects) return;

  ReaProject *proj = EnumProjects(-1, NULL, 0);  // -1 == active project
  if (!proj) {
    if (ShowMessageBox) ShowMessageBox("No project is open.", "ReaConvert example", 0);
    return;
  }

  reaconv::ProjectInfo info;
  reaconv::RppParser parser;
  if (!parser.Extract(proj, info)) {
    if (ShowMessageBox) ShowMessageBox("ReaConvert example: could not read the project.", "ReaConvert example", 0);
    return;
  }

  // output path: <project directory>/<project name>.json
  std::string outDir, baseName;
  char projFile[4096] = {0};
  if (GetSetProjectInfo_String) GetSetProjectInfo_String(proj, "PROJECT_FILE", projFile, false);

  if (projFile[0]) {
    // saved project: write next to the .rpp
    std::string pf(projFile);
    size_t slash = pf.find_last_of("/\\");
    outDir = (slash == std::string::npos) ? std::string(".") : pf.substr(0, slash);
    baseName = (slash == std::string::npos) ? pf : pf.substr(slash + 1);
    size_t dot = baseName.find_last_of('.');
    if (dot != std::string::npos) baseName.erase(dot);
  } else {
    // unsaved project: fall back to the recording directory, then the cwd
    char recDir[4096] = {0};
    if (GetProjectPath) GetProjectPath(recDir, sizeof(recDir));
    outDir = recDir[0] ? std::string(recDir) : std::string(".");

    char name[4096] = {0};
    if (GetSetProjectInfo_String) GetSetProjectInfo_String(proj, "PROJECT_NAME", name, false);
    baseName = name[0] ? name : "untitled";
  }
  std::string outPath = StripTrailingSlash(outDir) + "/" + SanitizeForFilename(baseName) + ".json";

  reaconv::JsonTimelineWriter writer;
  if (!writer.Write(info, outPath)) {
    std::string msg = "ReaConvert example: export failed.\n\n";
    msg += writer.LastError();
    if (ShowMessageBox) ShowMessageBox(msg.c_str(), "ReaConvert example", 0);
    return;
  }

  if (ShowConsoleMsg) {
    char buf[512];
    snprintf(buf, sizeof(buf), "!SHOW:ReaConvert example: wrote %s -- %d track(s), %d clip(s)\n",
             outPath.c_str(), (int)info.tracks.size(), CountClips(info));
    ShowConsoleMsg(buf);
  }
}

// called before every main-section action; we only handle our own command
static bool OnCommand(int command, int flag) {
  if (g_exportCommandId && command == g_exportCommandId) {
    ExportActiveProject();
    return true;
  }
  return false;
}

static custom_action_register_t g_exportAction = {0, "reaconv_example_export_json",
                                                  "ReaConvert example: export project as JSON timeline", NULL};

// -------------------------------------------------------------- entrypoint ---

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance,
                                                                reaper_plugin_info_t *rec) {
  if (!rec) return 0;  // our DLL is being unloaded

  if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc || !rec->Register) return 0;

  // hand REAPER's API function table to ReaConvert -- the only setup the
  // library needs
  if (ReaConvert_LoadAPI(rec->GetFunc) != 0) return 0;

  // import: makes *.json show up in File > Open / Insert
  if (!rec->Register("projectimport", (void *)&g_importRegister)) return 0;

  // export: an action the user can run or bind from the Actions list
  g_exportCommandId = rec->Register("custom_action", (void *)&g_exportAction);
  if (g_exportCommandId) rec->Register("hookcommand", (void *)OnCommand);

  return 1;
}

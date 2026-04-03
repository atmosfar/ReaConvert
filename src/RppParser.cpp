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

#include <algorithm>
#include <functional>
#include <stdint.h>
#include <sstream>
#include <vector>

#include "RppParser.h"
#include "common/ReaConvUtils.h"

namespace reaconv {

RppParser::RppParser() {}
RppParser::~RppParser() {}

static void ExtractEnvelopePoints(TrackEnvelope* env, Envelope& hubEnv,
                                  int sampleRate, int envMode, bool isVolume,
                                  bool isFX) {
  if (!env || !CountEnvelopePoints || !GetEnvelopePoint) return;

  auto processPoints = [&](int itemIdx) {
    int num_pts = (CountEnvelopePointsEx) ? CountEnvelopePointsEx(env, itemIdx)
                                          : CountEnvelopePoints(env);
    for (int i = 0; i < num_pts; ++i) {
      double time, value, tension;
      int shape;
      bool selected;
      bool success =
          (GetEnvelopePointEx)
              ? GetEnvelopePointEx(env, itemIdx, i, &time, &value, &shape,
                                   &tension, &selected)
              : GetEnvelopePoint(env, i, &time, &value, &shape, &tension,
                                 &selected);
      if (success) {
        double finalVal = value;
        if (isVolume && ScaleFromEnvelopeMode) {
          finalVal = ScaleFromEnvelopeMode(envMode, value);
        } else if (isFX) {
          if (hubEnv.maxValue != hubEnv.minValue) {
            finalVal =
                (value - hubEnv.minValue) / (hubEnv.maxValue - hubEnv.minValue);
          } else {
            finalVal = 0.0;
          }
        }
        Keyframe kf;
        kf.positionSamples = (long long)(time * sampleRate);
        kf.value = finalVal;
        kf.type = std::to_string(shape);
        hubEnv.keyframes.push_back(kf);
      }
    }
  };

  processPoints(-1);  // Main envelope

  if (CountAutomationItems) {
    int num_ai = CountAutomationItems(env);
    for (int ai = 0; ai < num_ai; ++ai) {
      processPoints(ai);
    }
  }

  std::sort(hubEnv.keyframes.begin(), hubEnv.keyframes.end(),
            [](const Keyframe& a, const Keyframe& b) {
              return a.positionSamples < b.positionSamples;
            });
}

static void PopulateHubEnvelope(TrackEnvelope* env, Envelope& hubEnv,
                                const std::string& name) {
  hubEnv.name = name;
  if (GetSetEnvelopeInfo_String) {
    char buf[128] = {0};
    if (GetSetEnvelopeInfo_String(env, "ACTIVE", buf, false))
      hubEnv.active = (buf[0] == '1');
    if (GetSetEnvelopeInfo_String(env, "ARM", buf, false))
      hubEnv.armed = (buf[0] == '1');
    if (GetSetEnvelopeInfo_String(env, "VISIBLE", buf, false))
      hubEnv.visible = (buf[0] == '1');
  }
}

/**
 * Parses REAPER version string (e.g. "7.63rc1/macOS-arm64")
 */
static bool IsVersionAtLeast(const std::string& versionStr, int major, int minor) {
  if (versionStr.empty()) return false;
  size_t slash = versionStr.find('/');
  std::string ver = (slash != std::string::npos) ? versionStr.substr(0, slash) : versionStr;
  size_t suffixStart = ver.find_first_not_of("0123456789.");
  if (suffixStart != std::string::npos) ver = ver.substr(0, suffixStart);
  size_t dot = ver.find('.');
  if (dot == std::string::npos) {
    try { return std::stoi(ver) > major || (std::stoi(ver) == major && minor == 0); } catch (...) { return false; }
  }
  try {
    int vMajor = std::stoi(ver.substr(0, dot));
    std::string minorStr = ver.substr(dot + 1);
    int vMinor = minorStr.empty() ? 0 : std::stoi(minorStr);
    return (vMajor > major) || (vMajor == major && vMinor >= minor);
  } catch (...) { return false; }
}

/**
 * Extracts AU decimal identifiers from a REAPER chunk line (<AU ...).
 */
static std::string ParseAUChunkLine(const std::string& line) {
  int quotesSeen = 0;
  size_t pos = 0;
  while (quotesSeen < 6 && pos < line.length()) {
    if (line[pos] == '\"') quotesSeen++;
    pos++;
  }
  if (quotesSeen < 6) return "";
  std::string idPart = line.substr(pos);
  size_t gt = idPart.find('>');
  if (gt != std::string::npos) idPart = idPart.substr(0, gt);
  idPart.erase(0, idPart.find_first_not_of(" \t\r\n"));
  size_t lastNonSpace = idPart.find_last_not_of(" \t\r\n");
  if (lastNonSpace != std::string::npos) idPart.erase(lastNonSpace + 1);
  return idPart;
}

static std::string NormalizeVST3ID(const std::string& raw) {
  if (raw.empty()) return "";
  std::string hex = raw;
  size_t start = hex.find('{');
  size_t end = hex.find('}');
  if (start != std::string::npos) {
    hex = (end != std::string::npos) ? hex.substr(start + 1, end - start - 1) : hex.substr(start + 1);
  }
  std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
  return "{" + hex + "}";
}

/**
 * Extracts FX name and developer info using GetFXName.
 */
static void ExtractFXNames(FX& hubFX, int index, 
                           std::function<bool(int, char*, int)> getFXName) {
  char buf[1024] = {0};
  if (getFXName(index, buf, sizeof(buf))) {
    std::string s = buf;
    if (hubFX.type == FXType::VST3 && s.length() > 6) s = s.substr(6);
    else if (hubFX.type == FXType::VST2 && s.length() > 5) s = s.substr(5);
    else if (hubFX.type == FXType::AU && s.length() > 4) s = s.substr(4);
    
    size_t op = s.find_first_of('(');
    size_t cl = s.find_first_of(')');
    if (op != std::string::npos && cl != std::string::npos && cl > op) {
      hubFX.developer = s.substr(op + 1, cl - op - 1);
      std::string cn = s.substr(0, op);
      while (!cn.empty() && isspace(cn.back())) cn.pop_back();
      hubFX.name = cn;
    } else {
      hubFX.name = s;
    }
  }
}


/**
 * Extracts Preset Data and handles fallback identification from the state chunk.
 */
static void ExtractFXPresetAndID(FX& hubFX, int index, 
                                 const std::string& chunk, 
                                 const std::string& sectionTag) {
  if (chunk.empty()) return;
  size_t fpos = chunk.find("<" + sectionTag);
  if (sectionTag == "FXCHAIN" && fpos == std::string::npos) fpos = chunk.find("<MASTERFXLIST");
  
  if (fpos != std::string::npos) {
    size_t cpos = fpos;
    int fxc = 0;
    while (cpos != std::string::npos) {
      size_t nfx = chunk.find("\n<", cpos);
      if (nfx == std::string::npos) break;
      if (fxc == index) {
        size_t lend = chunk.find("\n", nfx + 1);
        if (lend != std::string::npos) {
          if (hubFX.type == FXType::AU && hubFX.effectID.empty()) {
            hubFX.effectID = ParseAUChunkLine(chunk.substr(nfx, lend - nfx));
          }
          size_t cst = lend + 1;
          size_t cen = chunk.find("\n>", cst);
          if (cen != std::string::npos) {
            std::string raw = chunk.substr(cst, cen - cst);
            raw.erase(std::remove(raw.begin(), raw.end(), '\n'), raw.end());
            raw.erase(std::remove(raw.begin(), raw.end(), '\r'), raw.end());
            std::vector<unsigned char> dec = reaconv::Base64Decode(raw);
            size_t off = 0;
            if (hubFX.type == FXType::AU && dec.size() > 52) off = 52;
            else if (hubFX.type == FXType::VST2 && dec.size() > 60) off = 60;
            else if (hubFX.type == FXType::VST3 && dec.size() > 68) off = 68;
            
            if (off > 0 && dec.size() > off) {
              hubFX.presetChunkData = reaconv::Base64Encode(dec.data() + off, (unsigned int)dec.size() - off);
            } else {
              hubFX.presetChunkData = raw;
            }
          }
        }
        break;
      }
      fxc++; cpos = nfx + 1;
    }
  }
}



static ColorRGB GetRGBFromNative(int color) {
  ColorRGB rgb;
  if (!(color & 0x1000000)) return rgb;
  int r, g, b;
  if (ColorFromNative) {
    ColorFromNative(color, &r, &g, &b);
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
    rgb.hasColor = true;
  }
  return rgb;
}

bool RppParser::Extract(ReaProject* proj, ProjectInfo& project) {
  if (!proj || !GetSetProjectInfo) return false;
  int envMode = (int)GetSetProjectInfo(proj, "REAPER_VOLENV_SCALING", 0, false);
  if (envMode == 0) envMode = 1;
  ExtractMetadata(proj, project);
  ExtractMarkers(proj, project);
  ExtractTracks(proj, project, envMode);
  return true;
}

void RppParser::ExtractMetadata(ReaProject* proj, ProjectInfo& project) {
  if (!GetSetProjectInfo) return;
  project.sampleRate = (int)GetSetProjectInfo(proj, "PROJECT_SRATE", 0, false);
  if (project.sampleRate <= 0) project.sampleRate = 44100;
  project.tempo = GetSetProjectInfo(proj, "PROJECT_TEMPO", 0, false);
  project.timeSigNumerator =
      (int)GetSetProjectInfo(proj, "PROJECT_TSIG_NUM", 0, false);
  project.timeSigDenominator =
      (int)GetSetProjectInfo(proj, "PROJECT_TSIG_DENOM", 0, false);
  if (project.tempo <= 0) project.tempo = 120.0;
  if (project.timeSigNumerator <= 0) project.timeSigNumerator = 4;
  if (project.timeSigDenominator <= 0) project.timeSigDenominator = 4;
  if (GetProjectName) {
    char buf[1024] = {0};
    GetProjectName(proj, buf, sizeof(buf));
    project.projectName = buf;
  }
  if (TimeMap_curFrameRate)
    project.frameRate = TimeMap_curFrameRate(proj, &project.dropFrame);
  if (GetProjectLength) {
    double len = GetProjectLength(proj);
    if (len <= 0) len = 60.0;
    project.durationSamples = (long long)(len * project.sampleRate);
  } else {
    project.durationSamples = (long long)(60.0 * project.sampleRate);
  }
}

void RppParser::ExtractMarkers(ReaProject* proj, ProjectInfo& project) {
  if (!EnumProjectMarkers3) return;
  int idx = 0;
  bool isrgn;
  double pos, rgnend;
  const char* name;
  int markindex;
  int color;
  while (EnumProjectMarkers3(proj, idx++, &isrgn, &pos, &rgnend, &name,
                             &markindex, &color) > 0) {
    Marker m;
    m.name = name ? name : "";
    m.index = markindex;
    m.color = GetRGBFromNative(color);
    m.positionSamples = (long long)(pos * project.sampleRate);
    m.durationSamples =
        isrgn ? (long long)((rgnend - pos) * project.sampleRate) : 0;
    project.markers.push_back(m);
  }
}

MediaClip RppParser::ExtractClipData(
    MediaTrack* tr, int itemIdx, int sampleRate,
    std::map<std::string, std::string>& filePathToID, int envMode) {
  MediaClip hubClip;
  if (!GetTrackMediaItem || !GetActiveTake) return hubClip;
  MediaItem* item = GetTrackMediaItem(tr, itemIdx);
  if (!item) return hubClip;
  MediaItem_Take* take = GetActiveTake(item);
  if (take) {
    char nameBuf[512] = {0};
    if (GetSetMediaItemTakeInfo_String)
      GetSetMediaItemTakeInfo_String(take, "P_NAME", nameBuf, false);
    hubClip.name = nameBuf;
    if (GetMediaItemInfo_Value) {
      double pos = GetMediaItemInfo_Value(item, "D_POSITION");
      double len = GetMediaItemInfo_Value(item, "D_LENGTH");
      hubClip.startPositionSamples = (long long)(pos * sampleRate);
      hubClip.endPositionSamples = (long long)((pos + len) * sampleRate);
      double autoIn = GetMediaItemInfo_Value(item, "D_FADEINLEN_AUTO");
      hubClip.fadeInDuration = (autoIn >= 0) ? autoIn : GetMediaItemInfo_Value(item, "D_FADEINLEN");
      double autoOut = GetMediaItemInfo_Value(item, "D_FADEOUTLEN_AUTO");
      hubClip.fadeOutDuration = (autoOut >= 0) ? autoOut : GetMediaItemInfo_Value(item, "D_FADEOUTLEN");
      hubClip.fadeInShape = (int)GetMediaItemInfo_Value(item, "C_FADEINSHAPE");
      hubClip.fadeOutShape = (int)GetMediaItemInfo_Value(item, "C_FADEOUTSHAPE");
      hubClip.volume = (float)GetMediaItemInfo_Value(item, "D_VOL");
      hubClip.mute = (GetMediaItemInfo_Value(item, "B_MUTE") != 0);
      hubClip.loop = (GetMediaItemInfo_Value(item, "B_LOOPSRC") != 0);
      hubClip.group = (int)GetMediaItemInfo_Value(item, "I_GROUPID");
      if (hubClip.group <= 0) hubClip.group = -1;
      hubClip.color =
          GetRGBFromNative((int)GetMediaItemInfo_Value(item, "I_CUSTOMCOLOR"));
    }
    if (GetMediaItemTakeInfo_Value) {
      hubClip.sourceOffsetSamples = (long long)(
          GetMediaItemTakeInfo_Value(take, "D_STARTOFFS") * sampleRate);
    }
    if (GetNumTakeMarkers && GetTakeMarker) {
      int numTM = GetNumTakeMarkers(take);
      for (int m = 0; m < numTM; ++m) {
        char tmBuf[1024] = {0};
        int col = 0;
        double tpos = GetTakeMarker(take, m, tmBuf, sizeof(tmBuf), &col);
        Marker marker;
        marker.name = tmBuf;
        marker.color = GetRGBFromNative(col);
        marker.positionSamples = (long long)(tpos * sampleRate);
        marker.durationSamples = 0;
        marker.index = m + 1;
        hubClip.markers.push_back(marker);
      }
    }
    if (GetMediaItemTake_Source && GetMediaSourceFileName) {
      PCM_source* src = GetMediaItemTake_Source(take);
      if (src) {
        char pathBuf[2048] = {0};
        GetMediaSourceFileName(src, pathBuf, sizeof(pathBuf));
        hubClip.filePath = pathBuf;
        
        std::string ext = "";
        size_t dot = hubClip.filePath.find_last_of('.');
        if (dot != std::string::npos) ext = hubClip.filePath.substr(dot + 1);
        hubClip.sourceFormat = reaconv::GetSourceFormat(ext);
      }
    }
    ExtractTakeFXChain(take, hubClip.fxChain, sampleRate, envMode);
    ExtractTakeEnvelopes(take, hubClip.envelopes, hubClip.fxChain, sampleRate,
                        envMode, hubClip.sourceOffsetSamples);
  }
  return hubClip;
}

void RppParser::ExtractFXChain(MediaTrack* tr, std::vector<FX>& fxChain,
                               int sampleRate, int envMode) {
  if (!TrackFX_GetCount || !TrackFX_GetFXName || !TrackFX_GetNamedConfigParm) return;
  int numFX = TrackFX_GetCount(tr);
  if (numFX <= 0) return;

  std::string chunk = "";
  if (GetTrackStateChunk) {
    static char scbuf[2 * 1024 * 1024];
    if (GetTrackStateChunk(tr, scbuf, sizeof(scbuf), false)) chunk = scbuf;
  }

  for (int i = 0; i < numFX; ++i) {
    FX hubFX;
    char buf[1024] = {0};
    hubFX.powered = TrackFX_GetEnabled ? TrackFX_GetEnabled(tr, i) : true;
    if (TrackFX_GetPreset && TrackFX_GetPreset(tr, i, buf, sizeof(buf)))
      hubFX.presetName = (std::string(buf) == "Default") ? "(Default)" : buf;

    if (TrackFX_GetNamedConfigParm(tr, i, "fx_type", buf, sizeof(buf))) {
      std::string t = buf;
      if (t == "VST3") hubFX.type = FXType::VST3;
      else if (t == "VST") hubFX.type = FXType::VST2;
      else if (t == "AU") hubFX.type = FXType::AU;
      else hubFX.type = FXType::Unknown;
    }

    // Identifiers
    std::string appVer = GetAppVersion ? GetAppVersion() : "";
    if (hubFX.type == FXType::AU && IsVersionAtLeast(appVer, 7, 63)) {
      if (TrackFX_GetNamedConfigParm(tr, i, "au_ids", buf, sizeof(buf))) hubFX.effectID = buf;
    } else if (hubFX.type == FXType::VST2 || hubFX.type == FXType::VST3) {
      if (TrackFX_GetNamedConfigParm(tr, i, "fx_ident", buf, sizeof(buf))) {
        std::string ident = buf;
        size_t lt = ident.find('<');
        std::string idPart = (lt != std::string::npos) ? ident.substr(lt + 1) : ident;
        if (hubFX.type == FXType::VST3) hubFX.effectID = NormalizeVST3ID(idPart);
        else hubFX.effectID = idPart;
      }
    }

    ExtractFXNames(hubFX, i, [&](int idx, char* b, int sz){ return TrackFX_GetFXName(tr, idx, b, sz); });

    ExtractFXPresetAndID(hubFX, i, chunk, "FXCHAIN");
    fxChain.push_back(hubFX);
  }
}

void RppParser::ExtractTakeFXChain(MediaItem_Take* tk,
                                   std::vector<FX>& fxChain,
                                   int sampleRate, int envMode) {
  if (!TakeFX_GetCount || !TakeFX_GetFXName || !TakeFX_GetNamedConfigParm) return;
  int numFX = TakeFX_GetCount(tk);
  if (numFX <= 0) return;

  std::string chunk = "";
  if (GetMediaItemTake_Item && GetItemStateChunk) {
    MediaItem* item = GetMediaItemTake_Item(tk);
    if (item) {
      static char scbuf[1024 * 1024];
      if (GetItemStateChunk(item, scbuf, sizeof(scbuf), false)) chunk = scbuf;
    }
  }

  for (int i = 0; i < numFX; ++i) {
    FX hubFX;
    char buf[1024] = {0};
    hubFX.powered = TakeFX_GetEnabled ? TakeFX_GetEnabled(tk, i) : true;
    if (TakeFX_GetPreset && TakeFX_GetPreset(tk, i, buf, sizeof(buf)))
      hubFX.presetName = (std::string(buf) == "Default") ? "(Default)" : buf;

    if (TakeFX_GetNamedConfigParm(tk, i, "fx_type", buf, sizeof(buf))) {
      std::string t = buf;
      if (t == "VST3") hubFX.type = FXType::VST3;
      else if (t == "VST") hubFX.type = FXType::VST2;
      else if (t == "AU") hubFX.type = FXType::AU;
      else hubFX.type = FXType::Unknown;
    }

    // Identifiers
    std::string appVer = GetAppVersion ? GetAppVersion() : "";
    if (hubFX.type == FXType::AU && IsVersionAtLeast(appVer, 7, 63)) {
      if (TakeFX_GetNamedConfigParm(tk, i, "au_ids", buf, sizeof(buf))) hubFX.effectID = buf;
    } else if (hubFX.type == FXType::VST2 || hubFX.type == FXType::VST3) {
      if (TakeFX_GetNamedConfigParm(tk, i, "fx_ident", buf, sizeof(buf))) {
        std::string ident = buf;
        size_t lt = ident.find('<');
        std::string idPart = (lt != std::string::npos) ? ident.substr(lt + 1) : ident;
        if (hubFX.type == FXType::VST3) hubFX.effectID = NormalizeVST3ID(idPart);
        else hubFX.effectID = idPart;
      }
    }

    ExtractFXNames(hubFX, i, [&](int idx, char* b, int sz){ return TakeFX_GetFXName(tk, idx, b, sz); });

    ExtractFXPresetAndID(hubFX, i, chunk, "TAKEFX");
    fxChain.push_back(hubFX);
  }
}

static bool ExtractBoolFromChunk(const std::string& chunk, const std::string& key) {
  size_t pos = chunk.find(key);
  if (pos != std::string::npos) {
    size_t valPos = pos + key.length();
    while (valPos < chunk.length() && isspace(chunk[valPos])) valPos++;
    if (valPos < chunk.length()) return chunk[valPos] == '1';
  }
  return false;
}

Track RppParser::ExtractTrackData(
    MediaTrack* tr, int i, int sampleRate, int envMode,
    std::map<std::string, std::string>& filePathToID) {
  Track hubTrack;
  if (GetMediaTrackInfo_Value) {
    hubTrack.trackNumber = (int)GetMediaTrackInfo_Value(tr, "IP_TRACKNUMBER");
    MediaTrack* parent =
        (MediaTrack*)(uintptr_t)GetMediaTrackInfo_Value(tr, "P_PARTRACK");
    hubTrack.parentTrackNumber =
        parent ? (int)GetMediaTrackInfo_Value(parent, "IP_TRACKNUMBER") : -1;
    hubTrack.mainSend = GetMediaTrackInfo_Value(tr, "B_MAINSEND") != 0;
    hubTrack.mute = GetMediaTrackInfo_Value(tr, "B_MUTE") != 0;
    hubTrack.solo = GetMediaTrackInfo_Value(tr, "I_SOLO") != 0;
    hubTrack.armed = GetMediaTrackInfo_Value(tr, "I_RECARM") != 0;
    hubTrack.monitoring = (int)GetMediaTrackInfo_Value(tr, "I_RECMON") != 0;
    hubTrack.automationMode = (int)GetMediaTrackInfo_Value(tr, "I_AUTOMODE");
    hubTrack.showInMixer = GetMediaTrackInfo_Value(tr, "B_SHOWINMIXER") != 0;
    hubTrack.showInArrange = GetMediaTrackInfo_Value(tr, "B_SHOWINTCP") != 0;
    hubTrack.volume = (float)GetMediaTrackInfo_Value(tr, "D_VOL");
    hubTrack.pan = (float)GetMediaTrackInfo_Value(tr, "D_PAN");
    int depth = (int)GetMediaTrackInfo_Value(tr, "I_FOLDERDEPTH");
    int numRec = GetTrackNumSends ? GetTrackNumSends(tr, -1) : 0;
    hubTrack.isBus = (depth == 1) || (numRec > 0);

    if (GetTrackStateChunk) {
      static char scbuf[2 * 1024 * 1024];
      if (GetTrackStateChunk(tr, scbuf, sizeof(scbuf), false)) {
        std::string chunk = scbuf;
        hubTrack.automationLaneOpenState = ExtractBoolFromChunk(chunk, "SHOW");
      }
    }

    for (int s = 0; s < numRec; ++s) {
      Receive rx;
      MediaTrack* srcTr = (MediaTrack*)(uintptr_t)GetTrackSendInfo_Value(
          tr, -1, s, "P_SRCTRACK");
      rx.sourceTrackIndex =
          srcTr ? (int)GetMediaTrackInfo_Value(srcTr, "IP_TRACKNUMBER") : -1;
      rx.mode = (int)GetTrackSendInfo_Value(tr, -1, s, "I_SENDMODE");
      rx.volume = (float)GetTrackSendInfo_Value(tr, -1, s, "D_VOL");
      rx.pan = (float)GetTrackSendInfo_Value(tr, -1, s, "D_PAN");
      ExtractReceiveEnvelopes(tr, s, rx.envelopes, sampleRate, envMode);
      hubTrack.receives.push_back(rx);
    }
  }
  if (GetSetMediaTrackInfo_String) {
    char buf[512] = {0};
    GetSetMediaTrackInfo_String(tr, "P_NAME", buf, false);
    hubTrack.name = buf;
  }
  if (GetTrackColor) hubTrack.color = GetRGBFromNative(GetTrackColor(tr));
  if (GetTrackNumMediaItems) {
    int numItems = GetTrackNumMediaItems(tr);
    for (int j = 0; j < numItems; ++j)
      hubTrack.clips.push_back(
          ExtractClipData(tr, j, sampleRate, filePathToID, envMode));
  }
  ExtractFXChain(tr, hubTrack.fxChain, sampleRate, envMode);
  ExtractTrackEnvelopes(tr, hubTrack.envelopes, hubTrack.fxChain, sampleRate,
                        envMode);
  return hubTrack;
}

MasterTrack RppParser::ExtractMasterData(MediaTrack* master, int sampleRate,
                                              int envMode) {
  MasterTrack hubMaster;
  if (!master) return hubMaster;
  if (GetMediaTrackInfo_Value) {
    hubMaster.trackNumber = (int)GetMediaTrackInfo_Value(master, "IP_TRACKNUMBER");
    hubMaster.volume = (float)GetMediaTrackInfo_Value(master, "D_VOL");
    hubMaster.pan = (float)GetMediaTrackInfo_Value(master, "D_PAN");
    if (GetMasterMuteSoloFlags) {
      int flags = GetMasterMuteSoloFlags();
      hubMaster.mute = (flags & 1) != 0;
      hubMaster.solo = (flags & 2) != 0;
      hubMaster.sumToMono = (flags & 4) != 0;
    } else {
      hubMaster.mute = GetMediaTrackInfo_Value(master, "B_MUTE") != 0;
      hubMaster.solo = GetMediaTrackInfo_Value(master, "I_SOLO") != 0;
      hubMaster.sumToMono = false;
    }
    hubMaster.armed = GetMediaTrackInfo_Value(master, "I_RECARM") != 0;
    hubMaster.monitoring = (int)GetMediaTrackInfo_Value(master, "I_RECMON") != 0;
    hubMaster.automationMode = (int)GetMediaTrackInfo_Value(master, "I_AUTOMODE");

    if (GetTrackStateChunk) {
      static char scbuf[2 * 1024 * 1024];
      if (GetTrackStateChunk(master, scbuf, sizeof(scbuf), false)) {
        std::string chunk = scbuf;
        hubMaster.automationLaneOpenState = ExtractBoolFromChunk(chunk, "SHOW");
      }
    }
  }
  char buf[512] = {0};
  if (GetSetMediaTrackInfo_String)
    GetSetMediaTrackInfo_String(master, "P_NAME", buf, false);
  hubMaster.name = buf[0] ? buf : "Mix";
  ExtractFXChain(master, hubMaster.fxChain, sampleRate, envMode);
  ExtractTrackEnvelopes(master, hubMaster.envelopes, hubMaster.fxChain,
                        sampleRate, envMode);
  return hubMaster;
}

void RppParser::ExtractTracks(ReaProject* proj, ProjectInfo& project,
                              int envMode) {
  if (!CountTracks || !GetTrack || !GetMasterTrack || !GetMediaTrackInfo_Value)
    return;
  int numTracks = CountTracks(proj);
  std::map<std::string, std::string> filePathToID;
  MediaTrack* masterTr = GetMasterTrack(proj);
  if (masterTr)
    project.masterTrack =
        ExtractMasterData(masterTr, project.sampleRate, envMode);
  for (int i = 0; i < numTracks; ++i) {
    MediaTrack* tr = GetTrack(proj, i);
    if (tr)
      project.tracks.push_back(
          ExtractTrackData(tr, i, project.sampleRate, envMode, filePathToID));
  }
}

void RppParser::ExtractTakeEnvelopes(MediaItem_Take* tk,
                                     std::vector<Envelope>& envelopes,
                                     std::vector<FX>& fxChain,
                                     int sampleRate, int envMode,
                                     long long sourceOffsetSamples) {
  if (!tk) return;

  // 1. Mixer Envelopes
  const char* mixerEnvs[] = {"Volume", "Pan", "Mute"};
  for (const char* name : mixerEnvs) {
    TrackEnvelope* env =
        GetTakeEnvelopeByName ? GetTakeEnvelopeByName(tk, name) : nullptr;
    if (env) {
      Envelope hubEnv;
      PopulateHubEnvelope(
          env, hubEnv,
          (std::string(name) == "Volume")
              ? "volume"
              : (std::string(name) == "Pan") ? "pan" : "mute");
      ExtractEnvelopePoints(env, hubEnv, sampleRate, envMode,
                            (hubEnv.name == "volume"), false);
      if (!hubEnv.keyframes.empty() || hubEnv.visible)
        envelopes.push_back(hubEnv);
    }
  }

  // 2. FX Parameter Envelopes
  if (TakeFX_GetCount && TakeFX_GetNumParams && TakeFX_GetEnvelope &&
      TakeFX_GetParamEx) {
    int numFX = TakeFX_GetCount(tk);
    for (int i = 0; i < numFX; ++i) {
      int numParams = TakeFX_GetNumParams(tk, i);
      for (int p = 0; p < numParams; ++p) {
        TrackEnvelope* env = TakeFX_GetEnvelope(tk, i, p, false);
        if (env) {
          Envelope hubEnv;
          TakeFX_GetParamEx(tk, i, p, &hubEnv.minValue, &hubEnv.maxValue,
                            &hubEnv.midValue);
          hubEnv.index = p;
          char pnbuf[512] = {0};
          if (TakeFX_GetParamName &&
              TakeFX_GetParamName(tk, i, p, pnbuf, sizeof(pnbuf)))
            hubEnv.name = pnbuf;
          else
            hubEnv.name = "Param " + std::to_string(p);
          PopulateHubEnvelope(env, hubEnv, hubEnv.name);

          ExtractEnvelopePoints(env, hubEnv, sampleRate, envMode, false, true);

          if (i < (int)fxChain.size())
            fxChain[i].paramEnvelopes.push_back(hubEnv);
        }
      }
    }
  }
}

void RppParser::ExtractTrackEnvelopes(MediaTrack* tr,
                                      std::vector<Envelope>& envelopes,
                                      std::vector<FX>& fxChain,
                                      int sampleRate, int envMode) {
  if (!tr) return;
  const char* mixerEnvs[] = {"Volume", "Pan", "Mute"};
  for (const char* name : mixerEnvs) {
    TrackEnvelope* env =
        GetTrackEnvelopeByName ? GetTrackEnvelopeByName(tr, name) : nullptr;
    if (env) {
      Envelope hubEnv;
      PopulateHubEnvelope(
          env, hubEnv,
          (std::string(name) == "Volume")
              ? "volume"
              : (std::string(name) == "Pan") ? "pan" : "mute");
      ExtractEnvelopePoints(env, hubEnv, sampleRate, envMode,
                            (hubEnv.name == "volume"), false);
      if (!hubEnv.keyframes.empty() || hubEnv.visible)
        envelopes.push_back(hubEnv);
    }
  }
  if (TrackFX_GetCount && TrackFX_GetNumParams && GetFXEnvelope &&
      TrackFX_GetParamEx) {
    int numFX = TrackFX_GetCount(tr);
    for (int i = 0; i < numFX; ++i) {
      int numParams = TrackFX_GetNumParams(tr, i);
      for (int p = 0; p < numParams; ++p) {
        TrackEnvelope* env = GetFXEnvelope(tr, i, p, false);
        if (env) {
          Envelope hubEnv;
          TrackFX_GetParamEx(tr, i, p, &hubEnv.minValue, &hubEnv.maxValue,
                             &hubEnv.midValue);
          hubEnv.index = p;
          char pnbuf[512] = {0};
          if (TrackFX_GetParamName &&
              TrackFX_GetParamName(tr, i, p, pnbuf, sizeof(pnbuf)))
            hubEnv.name = pnbuf;
          else
            hubEnv.name = "Param " + std::to_string(p);
          PopulateHubEnvelope(env, hubEnv, hubEnv.name);
          ExtractEnvelopePoints(env, hubEnv, sampleRate, envMode, false, true);
          if (i < (int)fxChain.size())
            fxChain[i].paramEnvelopes.push_back(hubEnv);
        }
      }
    }
  }
}

void RppParser::ExtractReceiveEnvelopes(MediaTrack* tr, int sendIdx,
                                        std::vector<Envelope>& envelopes,
                                        int sampleRate, int envMode) {
  if (!GetTrackSendInfo_Value) return;
  const struct { const char* apiName; const char* hubName; } mixerEnvs[] = {
    {"<VOLENV", "volume"},
    {"<PANENV", "pan"},
    {"<MUTEENV", "mute"}
  };
  for (const auto& envInfo : mixerEnvs) {
    std::string envName = "P_ENV:";
    envName += envInfo.apiName;
    TrackEnvelope* env =
        (TrackEnvelope*)(uintptr_t)GetTrackSendInfo_Value(tr, -1, sendIdx,
                                                          envName.c_str());
    if (env) {
      Envelope hubEnv;
      PopulateHubEnvelope(env, hubEnv, envInfo.hubName);
      ExtractEnvelopePoints(env, hubEnv, sampleRate, envMode,
                            (hubEnv.name == "volume"), false);
      if (!hubEnv.keyframes.empty() || hubEnv.visible)
        envelopes.push_back(hubEnv);
    }
  }
}

}  // namespace reaconv


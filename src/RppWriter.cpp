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

#include "RppWriter.h"
#include "common/ReaConvUtils.h"
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <algorithm>

namespace reaconv {

RppWriter::RppWriter() {}
RppWriter::~RppWriter() {}

static int GetNativeFromColorRGB(const ColorRGB& rgb) {
    if (!rgb.hasColor) return 0;
    if (ColorToNative) {
#ifdef _MSC_VER
        return ColorToNative(rgb.r, rgb.g, rgb.b) | 0x1000000;
#else
        return ColorToNative(rgb.b, rgb.g, rgb.r) | 0x1000000;
#endif
    }
    // Fallback BGR
    return (rgb.b << 16) | (rgb.g << 8) | rgb.r | 0x1000000;
}

void RppWriter::Write(const ProjectInfo& project, ProjectStateContext* genstate) {
    genstate->AddLine("<REAPER_PROJECT 0.1");
    WriteMetadata(project, genstate);
    WriteMarkers(project, genstate);
    WriteTracks(project, genstate);
    genstate->AddLine(">");
}

// --- Envelope Helpers ---

static void WriteStandardEnvelope(ProjectStateContext* genstate, const Envelope& env, const char* chunkTag, int sampleRate, double timeOffset = 0.0) {
    if (env.keyframes.empty()) return;
    genstate->AddLine("    <%s", chunkTag);
    genstate->AddLine("      ACT %d -1", env.active ? 1 : 0);
    genstate->AddLine("      VIS %d 1 1.0", env.visible ? 1 : 0);
    genstate->AddLine("      LANEHEIGHT 0 0");
    genstate->AddLine("      ARM %d", env.armed ? 1 : 0);
    genstate->AddLine("      DEFSHAPE 0 -1 -1");
    if (std::string(chunkTag).find("VOL") != std::string::npos) genstate->AddLine("      VOLTYPE 1");
    for (const auto& kf : env.keyframes) {
        double time = (double)kf.positionSamples / sampleRate;
        double val = kf.value;
        int shape = 0;
        try { shape = std::stoi(kf.type); } catch(...) {}
        genstate->AddLine("      PT %.6f %.6f %d 0 0 0", time, val, shape);
    }
    genstate->AddLine("    >");
}

static void WriteMuteEnvelope(ProjectStateContext* genstate, const Envelope& env, const char* chunkTag, int sampleRate, double timeOffset = 0.0) {
    if (env.keyframes.empty()) return;
    genstate->AddLine("    <%s", chunkTag);
    genstate->AddLine("      ACT %d -1", env.active ? 1 : 0);
    genstate->AddLine("      VIS %d 1 1.0", env.visible ? 1 : 0);
    genstate->AddLine("      LANEHEIGHT 0 0");
    genstate->AddLine("      ARM %d", env.armed ? 1 : 0);
    genstate->AddLine("      DEFSHAPE 1 -1 -1");
    
    for (const auto& kf : env.keyframes) {
        double time = (double)kf.positionSamples / sampleRate;
        genstate->AddLine("      PT %.6f %.6f 1 0 0 0", time, kf.value);
    }
    genstate->AddLine("    >");
}

void RppWriter::WriteMetadata(const ProjectInfo& project, ProjectStateContext* genstate) {
    double effectiveTempo = (project.tempo > 0) ? project.tempo : 120.0;
    int effectiveSigNum = (project.timeSigNumerator > 0) ? project.timeSigNumerator : 4;
    int effectiveSigDen = (project.timeSigDenominator > 0) ? project.timeSigDenominator : 4;

    genstate->AddLine("  SAMPLERATE %d 0 0", project.sampleRate);
    genstate->AddLine("  TEMPO %.6f %d %d 0", effectiveTempo, effectiveSigNum, effectiveSigDen);
    
    // TIMEMODE mapping
    int fpsIdx = 0;
    int intFps = (int)std::round(project.frameRate);
    int flag = 0;

    if (std::abs(project.frameRate - 23.976) < 0.01) {
        fpsIdx = 0; intFps = 24; flag = 2;
    } else if (intFps == 24) {
        fpsIdx = 1; intFps = 24; flag = 0;
    } else if (intFps == 25) {
        fpsIdx = 2; intFps = 25; flag = 0;
    } else if (std::abs(project.frameRate - 29.97) < 0.01) {
        if (project.dropFrame) { fpsIdx = 3; intFps = 30; flag = 1; }
        else { fpsIdx = 4; intFps = 30; flag = 2; }
    } else if (intFps == 30) {
        fpsIdx = 5; intFps = 30; flag = 0;
    } else if (intFps == 48) {
        fpsIdx = 7; intFps = 48; flag = 0;
    } else if (intFps == 50) {
        fpsIdx = 8; intFps = 50; flag = 0;
    } else if (intFps == 60) {
        fpsIdx = 9; intFps = 60; flag = 0;
    } else if (intFps == 75) {
        fpsIdx = 6; intFps = 75; flag = 0;
    } else {
        // use 24 as fallback if unknown
        fpsIdx = 1; intFps = 24; flag = 0;
    }

    genstate->AddLine("  TIMEMODE 0 %d 0 %d %d 0 0 0", fpsIdx, intFps, flag);

    int masterMuteSolo = 0;
    if (project.masterTrack.mute) masterMuteSolo |= 1;
    if (project.masterTrack.solo) masterMuteSolo |= 2;
    if (project.masterTrack.sumToMono) masterMuteSolo |= 4;
    genstate->AddLine("  MASTERMUTESOLO %d", masterMuteSolo);
    genstate->AddLine("  MASTERTRACKVIEW %d 0.6667 0.5 0.5 0 0 0 0 0 0 0 0 0 0 0", project.masterTrack.visible ? 1 : 0);
    genstate->AddLine("  MASTER_VOLUME %.14f 0 -1 -1 1", project.masterTrack.volume);

    for (const auto& env : project.masterTrack.envelopes) {
        if (env.name == "volume") WriteStandardEnvelope(genstate, env, "MASTERVOLENV2", project.sampleRate);
        else if (env.name == "pan") WriteStandardEnvelope(genstate, env, "MASTERPANENV2", project.sampleRate);
        // Note: Master track does not support mute envelopes in REAPER
    }

    if (!project.masterTrack.fxChain.empty()) {
        genstate->AddLine("  <MASTERFXLIST");
        genstate->AddLine("    LASTSEL 0");
        genstate->AddLine("    DOCKED 0");
        genstate->AddLine("    BYPASS 0 0 0");
        for (const auto& fx : project.masterTrack.fxChain) WriteFX(genstate, fx, project.sampleRate);
        genstate->AddLine("  >");
    }
}

void RppWriter::WriteMarkers(const ProjectInfo& project, ProjectStateContext* genstate) {
    for (const auto& marker : project.markers) {
        double startTime = (double)marker.positionSamples / project.sampleRate;
        int col = GetNativeFromColorRGB(marker.color);

        if (marker.durationSamples == 0) {
            genstate->AddLine("  MARKER %d %.6f \"%s\" 0 %d 1 R", marker.index, startTime, marker.name.c_str(), col);
        } else {
            genstate->AddLine("  MARKER %d %.6f \"%s\" 1 %d 1 R", marker.index, startTime, marker.name.c_str(), col);
            double endTime = (double)(marker.positionSamples + marker.durationSamples) / project.sampleRate;
            genstate->AddLine("  MARKER %d %.6f \"\" 1", marker.index, endTime);
        }
    }
}

void RppWriter::WriteFX(ProjectStateContext* genstate, const FX& fx, int sampleRate) {
    std::vector<unsigned char> decoded = reaconv::Base64Decode(fx.presetChunkData);
    if (decoded.empty()) return;

    std::vector<unsigned char> payload = decoded;
    unsigned int payloadSize = (unsigned int)payload.size();
    std::vector<unsigned char> finalChunk;

    switch (fx.type) {
        case FXType::AU: {
            unsigned char wrapper[52] = {
                0xE9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00
            };
            wrapper[48] = payloadSize & 0xFF; wrapper[49] = (payloadSize >> 8) & 0xFF; wrapper[50] = (payloadSize >> 16) & 0xFF; wrapper[51] = (payloadSize >> 24) & 0xFF;
            
            finalChunk.insert(finalChunk.end(), wrapper, wrapper + 52);
            finalChunk.insert(finalChunk.end(), payload.begin(), payload.end());
            
            genstate->AddLine("    <AU \"AU: %s\" \"%s\" \"\" %s", fx.name.c_str(), fx.name.c_str(), fx.effectID.c_str());
            break;
        }

        case FXType::VST2:
        case FXType::VST3: {
            unsigned char wrapper[60] = {
                0x00, 0x00, 0x00, 0x00, 0xee, 0x5e, 0xed, 0xfe, 0x02, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };

            if (fx.type == FXType::VST2) {
                // --- VST2 Specific Logic ---
                try {
                    unsigned int magic = (unsigned int)std::stoul(fx.effectID);
                    wrapper[0] = magic & 0xFF; wrapper[1] = (magic >> 8) & 0xFF; wrapper[2] = (magic >> 16) & 0xFF; wrapper[3] = (magic >> 24) & 0xFF;
                } catch(...) {}

                unsigned int D = payloadSize + 1;
                wrapper[48] = D & 0xFF; wrapper[49] = (D >> 8) & 0xFF; wrapper[50] = (D >> 16) & 0xFF; wrapper[51] = (D >> 24) & 0xFF;
                wrapper[52] = 0x01; wrapper[53] = 0x00; wrapper[54] = 0x00; wrapper[55] = 0x00;
                wrapper[56] = 0x00; wrapper[57] = 0x00; wrapper[58] = 0x10; wrapper[59] = 0x00; 

                finalChunk.insert(finalChunk.end(), wrapper, wrapper + 60);
                finalChunk.insert(finalChunk.end(), payload.begin(), payload.end());
                finalChunk.push_back(0); 
            } else {
                // --- VST3 Specific Logic ---
                wrapper[0] = 0xE6; wrapper[1] = 0x2E; wrapper[2] = 0xFE; wrapper[3] = 0x14; 
                unsigned int D = payloadSize + 16;
                wrapper[48] = D & 0xFF; wrapper[49] = (D >> 8) & 0xFF; wrapper[50] = (D >> 16) & 0xFF; wrapper[51] = (D >> 24) & 0xFF;
                wrapper[52] = 0x01; wrapper[53] = 0x00; wrapper[54] = 0x00; wrapper[55] = 0x00;
                wrapper[56] = 0xFF; wrapper[57] = 0xFF; wrapper[58] = 0x10; wrapper[59] = 0x00;
                
                unsigned int internalSize = (payloadSize + 1) & ~1;
                unsigned char dataHeader[8] = { (unsigned char)(internalSize & 0xFF), (unsigned char)((internalSize >> 8) & 0xFF), 
                  (unsigned char)((internalSize >> 16) & 0xFF), (unsigned char)((internalSize >> 24) & 0xFF), 0x01, 0x00, 0x00, 0x00 };
                
                finalChunk.insert(finalChunk.end(), wrapper, wrapper + 60);
                finalChunk.insert(finalChunk.end(), dataHeader, dataHeader + 8);
                finalChunk.insert(finalChunk.end(), payload.begin(), payload.end());
                for(int i=0; i<8; ++i) finalChunk.push_back(0);
                if (internalSize > payloadSize) finalChunk.push_back(0);
            }

            std::string pluginPrefix = (fx.type == FXType::VST3) ? "VST3: " : "VST: ";
            std::string fileExt = (fx.type == FXType::VST3) ? ".vst3" : ".vst";
            
            std::string v2id = "0", v3id = "";
            if (fx.type == FXType::VST2) v2id = fx.effectID;
            else if (fx.type == FXType::VST3) {
                v2id = "352202470";
                v3id = fx.effectID;
                // Remove braces if present before writing to RPP
                if (!v3id.empty() && v3id.front() == '{') v3id = v3id.substr(1);
                if (!v3id.empty() && v3id.back() == '}') v3id.pop_back();
            }

            genstate->AddLine("    <VST \"%s%s\" \"%s%s\" 0 \"\" %s{%s} \"\"", 
                pluginPrefix.c_str(), fx.name.c_str(), fx.name.c_str(), fileExt.c_str(), 
                v2id.c_str(), v3id.c_str());
            break;
        }

        default:
            genstate->AddLine("    <");
            break;
    }

    std::string encoded = reaconv::Base64Encode(finalChunk.data(), (unsigned int)finalChunk.size());
    for (size_t i = 0; i < encoded.length(); i += 80) {
      genstate->AddLine("      %s", encoded.substr(i, 80).c_str());
    }


    // Construct the secondary Base64 line for the preset name
    std::vector<unsigned char> nameChunk;
    if (fx.presetName == "Default" || fx.presetName == "default" || fx.presetName == "(Default)") {
        nameChunk.assign(6, 0); // "AAAAAAAA"
    } else {
        nameChunk.push_back(0); // Leading null
        for (char c : fx.presetName) nameChunk.push_back((unsigned char)c);
        nameChunk.push_back(0); // Trailing null
        while (nameChunk.size() % 3 != 0) nameChunk.push_back(0); // Pad for alignment
    }
    std::string encodedName = reaconv::Base64Encode(nameChunk.data(), (unsigned int)nameChunk.size());
    if(!fx.presetName.empty()) {
      genstate->AddLine("      %s", encodedName.c_str());
    }

    genstate->AddLine("    >");
    
    std::string outPreset = fx.presetName;
    if (outPreset == "(Default)" || outPreset.empty()) {
      outPreset = "Default";
      genstate->AddLine("    PRESETNAME \"%s\"", outPreset.c_str());
    }

    for (const auto& env : fx.paramEnvelopes) {
        genstate->AddLine("    <PARMENV %d %.15g %.15g %.15g \"%s / %s\"", env.index, env.minValue, env.maxValue, env.midValue, env.name.c_str(), fx.name.c_str());
        genstate->AddLine("      ACT %d -1", env.active ? 1 : 0);
        genstate->AddLine("      VIS %d 1 1", env.visible ? 1 : 0);
        genstate->AddLine("      LANEHEIGHT 0 0");
        genstate->AddLine("      ARM %d", env.armed ? 1 : 0);
        genstate->AddLine("      DEFSHAPE 0 -1 -1");
        for (const auto& kf : env.keyframes) {
            double time = (double)kf.positionSamples / sampleRate;
            int shape = 0;
            try { shape = std::stoi(kf.type); } catch(...) {}
            genstate->AddLine("      PT %.6f %.6f %d 0 0 0", time, kf.value, shape);
        }
        genstate->AddLine("    >");
    }
}

void RppWriter::WriteTracks(const ProjectInfo& project, ProjectStateContext* genstate) {
    for (const auto& track : project.tracks) {
        genstate->AddLine("  <TRACK");
        genstate->AddLine("    NAME \"%s\"", track.name.c_str());
        genstate->AddLine("    PEAKCOL %d", GetNativeFromColorRGB(track.color));
        genstate->AddLine("    SHOWINMIX %d 0.6667 0.5 %d 0.5 0 0 0 %d", track.showInMixer ? 1 : 0, track.showInArrange ? 1 : 0, track.pinned ? 1 : 0);
        genstate->AddLine("    AUTOMODE %d", track.automationMode);
        float volpan_v = track.volume;
        bool hasVolEnv = false;
        for(auto& e : track.envelopes) if(e.name == "volume") hasVolEnv = true;
        if(hasVolEnv) volpan_v = 1.0f;
        genstate->AddLine("    VOLPAN %.6f %.6f -1.000 -1.000 1", volpan_v, track.pan);
        genstate->AddLine("    MUTESOLO %d %d 0", track.mute ? 1 : 0, track.solo ? 1 : 0);
        genstate->AddLine("    REC %d 0 %d 0 0 0 0 0", track.armed ? 1 : 0, track.monitoring ? 1 : 0);
        if (track.mainSend) genstate->AddLine("    MAINSEND 1 0");
        else genstate->AddLine("    MAINSEND 0 0");

        if (!track.fxChain.empty()) {
            genstate->AddLine("    <FXCHAIN");
            genstate->AddLine("      SHOW 0");
            genstate->AddLine("      LASTSEL 0");
            genstate->AddLine("      DOCKED 0");
            genstate->AddLine("      BYPASS 0 0 0");
            for (const auto& fx : track.fxChain) WriteFX(genstate, fx, project.sampleRate);
            genstate->AddLine("    >");
        }

        for (const auto& env : track.envelopes) {
            if (env.name == "volume") WriteStandardEnvelope(genstate, env, "VOLENV2", project.sampleRate);
            else if (env.name == "pan") WriteStandardEnvelope(genstate, env, "PANENV2", project.sampleRate);
            else if (env.name == "mute") WriteMuteEnvelope(genstate, env, "MUTEENV", project.sampleRate);
        }
        for (const auto& rx : track.receives) {
            genstate->AddLine("    AUXRECV %d %d %.6f %.6f 0 0 0 0 0 -1:U 0 -1 ''", rx.sourceTrackIndex - 1, rx.mode, rx.volume, rx.pan);
            for (const auto& env : rx.envelopes) {
                if (env.name == "volume") WriteStandardEnvelope(genstate, env, "AUXVOLENV", project.sampleRate);
                else if (env.name == "pan") WriteStandardEnvelope(genstate, env, "AUXPANENV", project.sampleRate);
                else if (env.name == "mute") WriteMuteEnvelope(genstate, env, "AUXMUTEENV", project.sampleRate);
            }
        }
        for (const auto& clip : track.clips) {
            double position = (double)clip.startPositionSamples / project.sampleRate;
            double length = (double)(clip.endPositionSamples - clip.startPositionSamples) / project.sampleRate;
            double soffs = (double)clip.sourceOffsetSamples / project.sampleRate;
            genstate->AddLine("    <ITEM");
            genstate->AddLine("      POSITION %.6f", position);
            genstate->AddLine("      LENGTH %.6f", length);
            genstate->AddLine("      SOFFS %.6f", soffs);
            if (clip.mute) genstate->AddLine("      MUTE 1 0");
            if (clip.color.hasColor) genstate->AddLine("      COLOR %d R", GetNativeFromColorRGB(clip.color));
            if (clip.group != -1) genstate->AddLine("      GROUP %d", clip.group);
            
            genstate->AddLine("      FADEIN %d %.6f 0.0", clip.fadeInShape, clip.fadeInDuration);
            genstate->AddLine("      FADEOUT %d %.6f 0.0", clip.fadeOutShape, clip.fadeOutDuration);
            float clip_v = clip.volume;
            bool hasClipVol = false;
            for(auto& e : clip.envelopes) if(e.name == "volume") hasClipVol = true;
            if(hasClipVol) clip_v = 1.0f;
            genstate->AddLine("      VOLPAN %.6f 0.0 1.0 -1.0", clip_v);
            genstate->AddLine("      LOOP %d", clip.loop ? 1 : 0);
            genstate->AddLine("      NAME \"%s\"", clip.name.c_str());
            
            for (const auto& marker : clip.markers) {
                double pos = (double)marker.positionSamples / project.sampleRate;
                int col = GetNativeFromColorRGB(marker.color);
                genstate->AddLine("      TKM %.14f \"%s\" %d 0 R", pos, marker.name.c_str(), col);
            }

            double clipTimeOffset = (double)clip.sourceOffsetSamples / project.sampleRate;
            for (const auto& env : clip.envelopes) {
                if (env.name == "volume") WriteStandardEnvelope(genstate, env, "VOLENV", project.sampleRate, clipTimeOffset);
                else if (env.name == "pan") WriteStandardEnvelope(genstate, env, "PANENV", project.sampleRate, clipTimeOffset);
            }

            if (!clip.fxChain.empty()) {
                genstate->AddLine("      <TAKEFX");
                genstate->AddLine("        SHOW 0");
                genstate->AddLine("        LASTSEL 0");
                genstate->AddLine("        DOCKED 0");
                genstate->AddLine("        BYPASS 0 0 0");
                for (const auto& fx : clip.fxChain) WriteFX(genstate, fx, project.sampleRate);
                genstate->AddLine("      >");
            }

            std::string format = GetSourceFormat(clip.filePath.substr(clip.filePath.find_last_of('.') + 1));
            genstate->AddLine("      <SOURCE %s", format.c_str());
            if (track.type == 1) genstate->AddLine("        VIDEO_DISABLED");
            else if (track.type == 2) genstate->AddLine("        AUDIO 0");
            genstate->AddLine("        FILE \"%s\" 1", clip.filePath.c_str());
            genstate->AddLine("      >");
            genstate->AddLine("    >");
        }
        genstate->AddLine("  >");
    }
}

} // namespace reaconv

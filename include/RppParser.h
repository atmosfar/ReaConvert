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

#ifndef RPP_PARSER_H
#define RPP_PARSER_H

#include <map>

#include "common/ProjectInfo.h"
#include "reaper_plugin_functions.h"

namespace reaconv {

class RppParser {
public:
    RppParser();
    ~RppParser();

    bool Extract(ReaProject* proj, ProjectInfo& project);

private:
    void ExtractMetadata(ReaProject* proj, ProjectInfo& project);
    void ExtractMarkers(ReaProject* proj, ProjectInfo& project);
    void ExtractTracks(ReaProject* proj, ProjectInfo& project, int envMode);
    
        Track ExtractTrackData(MediaTrack* tr, int i, int sampleRate, int envMode, 
                                   std::map<std::string, std::string>& filePathToID);
        MasterTrack ExtractMasterData(MediaTrack* master, int sampleRate, int envMode);

    MediaClip ExtractClipData(MediaTrack* tr, int itemIdx, int sampleRate, std::map<std::string, std::string>& filePathToID, int envMode);
    void ExtractTrackEnvelopes(MediaTrack* tr, std::vector<Envelope>& envelopes, std::vector<FX>& fxChain, int sampleRate, int envMode);
    void ExtractTakeEnvelopes(MediaItem_Take* tk, std::vector<Envelope>& envelopes, std::vector<FX>& fxChain, int sampleRate, int envMode, long long sourceOffsetSamples);
    void ExtractFXChain(MediaTrack* tr, std::vector<FX>& fxChain, int sampleRate, int envMode);
    void ExtractTakeFXChain(MediaItem_Take* tk, std::vector<FX>& fxChain, int sampleRate, int envMode);
    void ExtractReceiveEnvelopes(MediaTrack* tr, int sendIdx, std::vector<Envelope>& envelopes, int sampleRate, int envMode);
};

} // namespace reaconv

#endif // RPP_PARSER_H

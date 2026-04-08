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

#ifndef PROJECT_INFO_H
#define PROJECT_INFO_H

#include <string>
#include <vector>

namespace reaconv {

struct ColorRGB {
    int r;
    int g;
    int b;
    bool hasColor;

    ColorRGB() : r(0), g(0), b(0), hasColor(false) {}
    ColorRGB(int red, int green, int blue) : r(red), g(green), b(blue), hasColor(true) {}

    bool operator==(const ColorRGB& other) const {
        return hasColor == other.hasColor && r == other.r && g == other.g && b == other.b;
    }
};

struct Keyframe {
    long long positionSamples;
    double value;
    std::string type; // "linear", "hold", "bezier", etc.
};

struct Envelope {
    std::string name;
    int index;
    bool active;
    bool armed;
    bool visible;
    double minValue;
    double maxValue;
    double midValue;
    double parameterValue;
    std::vector<Keyframe> keyframes;

    Envelope() : index(0), active(true), armed(false), visible(true), minValue(0.0), maxValue(1.0), midValue(0.5), parameterValue(0.0) {}
};

struct Marker {
    std::string name;
    long long positionSamples;
    long long durationSamples; // 0 for point markers
    int index;
    ColorRGB color;
};

struct Receive {
    int sourceTrackIndex;
    int mode; // 0=Post Fader, 1=Pre FX, 3=Pre Fader
    float volume;
    float pan;
    std::vector<Envelope> envelopes;
};

enum class FXType {
    VST2,
    VST3,
    AU,
    Unknown
};

struct FX {
    std::string name;
    std::string developer;
    std::string presetName;
    std::string effectID;
    std::string presetChunkData;
    bool powered;
    FXType type;
    std::vector<Envelope> paramEnvelopes;
};

struct MediaClip {
    std::string name;
    std::string filePath;
    std::string fileID;
    std::string sourceFormat;
    long long startPositionSamples;
    long long endPositionSamples;
    long long sourceOffsetSamples;
    float volume;
    float gain;
    int group;
    bool mute;
    bool loop;
    ColorRGB color;

    MediaClip() : startPositionSamples(0), endPositionSamples(0), sourceOffsetSamples(0), 
                  volume(1.0f), gain(1.0f), group(-1), mute(false), loop(true), fadeInDuration(0.0), 
                  fadeInShape(0), fadeOutDuration(0.0), fadeOutShape(0) {}
    
    // Fades (in seconds)
    double fadeInDuration;
    int fadeInShape;
    double fadeOutDuration;
    int fadeOutShape;
    
    // Envelopes
    std::vector<Envelope> envelopes;
    std::vector<Marker> markers;
    std::vector<FX> fxChain;
};

struct Track {
    std::string name;
    int trackNumber;
    int parentTrackNumber;
    ColorRGB color;
    bool isBus;
    bool mainSend;
    bool mute;
    bool solo;
    bool armed;
    bool monitoring;
    bool pinned;
    bool showInMixer;
    bool showInArrange;
    bool automationLaneOpenState;
    int automationMode;
    int type; // 0=default, 1=audio-only, 2=video-only
    float volume;
    float pan;
    float width;
    
    std::vector<MediaClip> clips;
    std::vector<Envelope> envelopes;
    std::vector<Receive> receives;
    std::vector<FX> fxChain;

    Track() : trackNumber(0), parentTrackNumber(0), isBus(false), mainSend(true), mute(false), solo(false), 
              armed(false), monitoring(false), pinned(false), showInMixer(true), 
              showInArrange(true), automationLaneOpenState(false), automationMode(0), type(0), volume(1.0f), 
              pan(0.0f), width(1.0f) {}
};

struct MasterTrack {
    std::string name;
    int trackNumber;
    float volume;
    float pan;
    bool visible;
    bool mute;
    bool solo;
    bool armed;
    bool monitoring;
    int automationMode;
    bool automationLaneOpenState;
    bool sumToMono;
    std::vector<FX> fxChain;
    std::vector<Envelope> envelopes;

    MasterTrack() : trackNumber(-1), volume(1.0f), pan(0.0f), visible(true), mute(false), 
                    solo(false), armed(false), monitoring(false), automationMode(0), 
                    automationLaneOpenState(false), sumToMono(false) {}
};

struct ProjectInfo {
    std::string projectName;
    int sampleRate;
    double frameRate;
    bool dropFrame;
    double tempo;
    int timeSigNumerator;
    int timeSigDenominator;
    long long durationSamples;
    
    MasterTrack masterTrack;
    std::vector<Marker> markers;
    std::vector<Track> tracks;
};

} // namespace reaconv

#endif // PROJECT_INFO_H

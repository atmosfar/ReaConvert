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

#ifndef REACONV_UTILS_H
#define REACONV_UTILS_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "reaper_plugin_functions.h"
#include "ProjectInfo.h"

#ifdef _WIN32
#include <objbase.h>
#endif

namespace reaconv {

// --- GUID Helper ---
inline std::string GetGUIDString() {
#if defined(SESX_TESTING)
    static int counter = 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "00000000-0000-0000-0000-%012d", counter++);
    return std::string(buf);
#else
    GUID g;
#ifdef _WIN32
    CoCreateGuid(&g);
#else
    if (SWELL_GenerateGUID) {
        SWELL_GenerateGUID(&g);
    } else {
        memset(&g, 0, sizeof(g));
    }
#endif
    char buf[64];
    snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
             (unsigned int)g.Data1, g.Data2, g.Data3, 
             (unsigned int)g.Data4[0], (unsigned int)g.Data4[1], (unsigned int)g.Data4[2], (unsigned int)g.Data4[3], 
             (unsigned int)g.Data4[4], (unsigned int)g.Data4[5], (unsigned int)g.Data4[6], (unsigned int)g.Data4[7]);
    return std::string(buf);
#endif
}

// --- Base64 Utilities ---

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

inline std::string Base64Encode(const unsigned char* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while((i++ < 3)) ret += '=';
    }
    return ret;
}

inline std::vector<unsigned char> Base64Decode(std::string const& encoded_string) {
    int in_len = (int)encoded_string.size();
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;

    while (in_len-- && ( encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i <4; i++) char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; (i < 3); i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    if (i) {
        for (j = i; j <4; j++) char_array_4[j] = 0;
        for (j = 0; j <4; j++) char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }
    return ret;
}

// --- Color Utilities ---

inline void HSVtoRGB(float h, float s, float v, int& r, int& g, int& b) {
    float f, p, q, t;
    int i;
    if (s == 0) {
        r = g = b = (int)(v * 255);
        return;
    }
    h /= 60.0f;
    i = (int)std::floor(h);
    f = h - i;
    p = v * (1.0f - s);
    q = v * (1.0f - (s * f));
    t = v * (1.0f - (s * (1.0f - f)));
    float rf, gf, bf;
    switch (i % 6) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        case 5: rf = v; gf = p; bf = q; break;
        default: rf = 0; gf = 0; bf = 0; break;
    }
    r = (int)(rf * 255); g = (int)(gf * 255); b = (int)(bf * 255);
}

inline ColorRGB HueToRGB(float track_hue) {
    if (track_hue == -1) return ColorRGB(); // Default color (no color)
    float hue = std::fmod(track_hue, 360.0f);
    int r, g, b;
    HSVtoRGB(hue, 0.5f, 1.0f, r, g, b);
    return ColorRGB(r, g, b);
}

inline ColorRGB NativeToRGB(int native_color) {
    if (!(native_color & 0x1000000)) return ColorRGB();
    int r = native_color & 0xFF;
    int g = (native_color >> 8) & 0xFF;
    int b = (native_color >> 16) & 0xFF;
    return ColorRGB(r, g, b);
}

inline int RGBToHue(const ColorRGB& rgb) {
    if (!rgb.hasColor) return -1;
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    float max_val = (std::max)({r, g, b}), min_val = (std::min)({r, g, b}), delta = max_val - min_val;
    float h = 0;
    if (delta > 0) {
        if (max_val == r) h = 60.0f * (float)std::fmod(((g - b) / delta), 6);
        else if (max_val == g) h = 60.0f * (((b - r) / delta) + 2);
        else if (max_val == b) h = 60.0f * (((r - g) / delta) + 4);
    }
    if (h < 0) h += 360;
    return (int)h;
}

// --- Mapping ---

inline std::string GetSourceFormat(const std::string& extension) {
    std::string ext = extension;
    for (auto & c: ext) c = (char)tolower(c);

    if (ext == "wav" || ext == "bwf" || ext == "rf64" || ext == "w64" ||
        ext == "aif" || ext == "aiff" || ext == "aifc" || ext == "caf" ||
        ext == "paf" || ext == "pcm" || ext == "raw" || ext == "pvf" ||
        ext == "snd" || ext == "au"  || ext == "sd2" || ext == "avr" ||
        ext == "htk" || ext == "iff" || ext == "mat" || ext == "mpc" ||
        ext == "sds" || ext == "sf"  || ext == "voc" || ext == "vox" ||
        ext == "wve" || ext == "xi") {
        return "WAVE";
    }
    if (ext == "mp3" || ext == "mp2") return "MP3";
    if (ext == "flac") return "FLAC";
    if (ext == "ogg" || ext == "oga") return "VORBIS";
    if (ext == "opus") return "OPUS";
    if (ext == "m4a" || ext == "mp4" || ext == "aac" || ext == "mov" ||
        ext == "m4v" || ext == "3gp" || ext == "3g2" || ext == "avi" ||
        ext == "wmv" || ext == "wma" || ext == "dv"  || ext == "flv" ||
        ext == "mpg" || ext == "mpeg"|| ext == "m1v" || ext == "m2v" ||
        ext == "r3d" || ext == "swf" || ext == "ape" || ext == "ac3" ||
        ext == "ec3") {
        return "VIDEO";
    }
    return "WAVE";
}

} // namespace reaconv

#endif // REACONV_UTILS_H

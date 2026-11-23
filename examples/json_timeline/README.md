# Example ReaConvert implementation: JSON timeline 

A minimal REAPER extension which demonstrates project import and export by ReaConvert using a trivial JSON timeline format:

| direction | pipeline |
|---|---|
| **import** `example.json` → REAPER project | `JsonTimelineParser` → `ProjectInfo` → `RppWriter` |
| **export** REAPER project → `example.json` | `RppParser` → `ProjectInfo` → `JsonTimelineWriter` |

## Build

```bash
git submodule update --init --recursive
cmake -S . -B build                                # examples are on by default
cmake --build build -j
```

## Use

*Import* - `File > Open...` and pick`sample_timeline.json` from this folder.

The extension only claims `.json` files that contain `"reaconv-timeline"` in their first 256 bytes, so it will not import irrelevant JSON.

*Export* - run the action *"ReaConvert example: export project as JSON timeline"*. It writes`<project directory>/<project name>.json` next to your `.rpp` and prints the path in the console.

## Schema

Everything is plain JSON. All times are seconds, all gains are linear (`1.0` = unity), `pan` is `-1..1`. Every field except `format` and`tracks` is optional.

```jsonc
{
  "format": "reaconv-timeline",   // required, the extension's file signature
  "version": 1,
  "name": "Example timeline",     // project name
  "sampleRate": 44100,
  "tempo": 96.0,
  "timeSignature": [4, 4],
  "markers": [                    // project markers / regions
    { "name": "Intro", "time": 0.0, "length": 2.0, "color": "#8888ff" },
    { "name": "Drop",  "time": 2.0 }             // no length => point marker
  ],
  "tracks": [
    {
      "name": "Low end",
      "color": "#ff9922",
      "volume": 0.9, "pan": 0.0,
      "mute": false, "solo": false, "recordArmed": false,
      "clips": [
        {
          "name": "tone A x2",
          "file": "media/tone_a.wav",   // absolute, or relative to this JSON file
          "start": 0.0,                 // item position
          "length": 2.0,                // item length
          "offset": 0.25,               // source start offset
          "volume": 1.0,                // item volume
          "gain": 1.4,                  // active-take gain
          "pan": -0.25,                 // clip pan (REAPER's active-take pan)
          "fadeIn": 0.005, "fadeOut": 0.05,
          "loop": true, "mute": false,
          "color": "#ff3333"
        }
      ]
    }
  ]
}
```

Unknown keys are ignored, missing keys fall back to the defaults shown above.

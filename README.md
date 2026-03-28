# ReaConvert

ReaConvert is a library for parsing and writing Reaper projects. It provides one half of the implementation for extensions supporting new project formats in Reaper.

### Structure:
* ProjectInfo struct - common representation of the media project, including clips, tracks, VSTs, markers, envelopes, routing etc.
* RppWriter class -  takes a ProjectInfo instance and generates a Reaper project, which is then imported
* RppParser class - populates a ProjectInfo instance from the project currently open in Reaper

### Usage:
`YourProjectFormatParser` -> `ProjectInfo` -> `RppWriter`

or

`RppParser` -> `ProjectInfo` -> `YourProjectFormatWriter`

### Credits:
The REAPER devs and community.


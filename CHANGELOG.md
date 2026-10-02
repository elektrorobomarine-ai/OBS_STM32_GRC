## v1.0.0 - Source formatting refresh
- Reformatted all implementation and header files for readability.
- Standardized C indentation to 4 spaces and removed tabs.
- Expanded minified one-line functions and structures.
- Added `.clang-format` and `.editorconfig`.
- Added `Docs/CODE_FORMATTING.md`.
- No intentional protocol or pin-assignment change.

# Changelog
## v1.0.0 — Operational release
- Completed 4-channel AD7768 SAI/DMA path, 128 frames per block.
- Matched OBS Runtime TCP ports 54300/54301 and binary bulk framing.
- Added NMEA-style command parser/checksum.
- Added UTC RTC handling and TIME1 command.
- Added WT901 parser.
- Added MS5837 depth/temperature driver.
- Added ADC leak/VBAT monitoring and relay control.
- Added FATFS USB BIN logger matching OBS binary log header/record layout.
- Added operational task topology and CubeMX integration requirements.

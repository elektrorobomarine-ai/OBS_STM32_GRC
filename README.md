# Robomarine OBS STM32H743 Firmware v1.0.0

Operational firmware source package matched to the established OBS Runtime interfaces.

## Implemented
- AD7768-4 configuration via SPI2
- AD7768 4-channel TDM acquisition via SAI1 + circular DMA
- 128-frame half-buffer (2048-byte raw payload), preserving 8-bit ADC status + 24-bit sample
- ADC diagnostics: channel-ID mismatch, error/status flags, SAI errors, measured sample rate
- TCP command/telemetry server port 54300, NMEA-style checksum framing
- TCP bulk ADC server port 54301, `OBS:` 12-byte header + 2048-byte payload
- Commands: relay 1/2, USB log start/stop, UTC time set
- USB FATFS binary logger compatible with `BINARY_LOG_FORMAT.txt`
- RTC treated as UTC; USB filenames use UTC
- WT901 UART parser (angle + gyro packets)
- MS5837 I2C driver (30BA default)
- leak + battery ADC monitoring
- FreeRTOS modular tasks

## Build boundary
This repository contains the complete application/driver/middleware code. STM32CubeIDE must generate the MCU startup/HAL/LwIP/USB/FATFS files for the exact STM32H743 package and board clock tree. Follow `Docs/CUBEMX_REQUIRED_SETTINGS.md` and insert `Docs/main_integration_snippet.c` into CubeMX USER CODE sections.

## Important bench assumptions
- AD7768 MCLK = 32.768 MHz
- target ADC rate = 1000 frames/s
- AD7768 format = one DOUT, 4 serialized 32-bit words/frame
- depth sensor = MS5837-30BA at default address
- Ethernet PHY = LAN8720 RMII
- UTC RTC policy

## Git
```bash
git init
git add .
git commit -m "release: OBS STM32H743 firmware v1.0.0"
git branch -M main
git remote add origin <repo-url>
git push -u origin main
git tag -a v1.0.0 -m "Operational release"
git push origin v1.0.0
```

## BIN record-header note
The current formal `BINARY_LOG_FORMAT.txt` declares a 34-byte record header although the listed fields occupy 30 bytes. v1.0.0 writes the declared 34-byte form (4 reserved trailing bytes). The current OBS Python reader auto-detects both 34-byte and packed 30-byte records.

# STM32CubeMX settings — OBS STM32H743 v1.0.0

## Mandatory middleware/peripherals
- FreeRTOS CMSIS-RTOS2
- LwIP, IPv4, TCP, sockets enabled
- FATFS
- USB Host MSC (USB OTG FS on PA11/PA12; external +5 V VBUS switch)
- RTC (calendar interpreted as UTC)
- SPI2 Mode 0, 8-bit, PB10/PB14/PB15; PB4 GPIO CS
- SAI1 Block A receiver, slave, 32-bit slots, 4 slots, 128-bit frame, external SCK/FS
- SAI1 RX circular DMA
- TIM1 CH1 PA8 per ADC START requirement
- USART3 115200 8N1, RX DMA normal; ReceiveToIdle is restarted by driver
- I2C1 PB8/PB9, 400 kHz recommended
- ADC1 scan PA0 then PC0, continuous DMA, 16-bit if available on selected H743 ADC mode
- ETH RMII + LAN8720

## Critical STM32H7 DMA/cache
The `.sai_dma_buffer` section must live in DMA-accessible SRAM, NOT DTCM.
Best operational configuration is MPU non-cacheable for this section. The driver also invalidates D-cache before reading each half.

Example linker section:
```
.sai_dma_buffer (NOLOAD) :
{
  . = ALIGN(32);
  *(.sai_dma_buffer)
  . = ALIGN(32);
} >RAM_D2
```

## SAI expected wire format
Each 32-bit word from AD7768-4 is retained unchanged:
- [31:24] status/header
- [23:0] signed conversion
- CH0, CH1, CH2, CH3 repeated
- 128 frames per half-buffer = 2048 bytes

## Network compatibility
TCP server 54300: NMEA-style command/telemetry
TCP server 54301: bulk binary (`OBS:` + uint32 sequence + uint32 payload_len + 2048 bytes)

## USB BIN compatibility
- file header: 20 bytes
- record header: 34 bytes
- raw payload: 2048 bytes
- 128 frames/record, 4 channels, 1 kHz
- filename UTC `OBS_YYYYMMDD_HHMMSS.bin`

## Items to bench-calibrate, not build blockers
- VBAT divider ratio
- leak threshold
- MS5837 atmospheric offset / seawater density
- AD7768 START waveform polarity/duty if board differs
- Ethernet PHY address/reset wiring

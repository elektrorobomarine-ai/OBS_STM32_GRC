# AD7768-4 Acquisition Setup — Firmware v0.2.0

## Locked v0.2 profile
This release intentionally supports one known-safe acquisition profile first:

- AD7768-4
- MCLK = 32.768 MHz
- SPI control mode
- FORMAT0 = 1, CH0..CH3 serialized on DOUT0
- Low-power mode
- MCLK divider = /32
- fMOD = 1.024 MHz
- Decimation = 1024
- ODR = 1000 Hz/channel
- CRC disabled
- Header enabled every conversion
- 32-bit slot = 8-bit header + 24-bit signed sample

Filter is configurable between wideband and Sinc5; firmware defaults to wideband.

## CubeMX: SPI2
- Master
- Full duplex
- Hardware NSS disabled; PB4 is software CS
- 8-bit data size (two bytes per 16-bit AD7768 transaction)
- MSB first
- SPI Mode 0: CPOL=Low, CPHA=1st edge.
- Keep SCLK well below the device/control-port limit.

## CubeMX: SAI1 Block A
Configure receiver as:
- Asynchronous slave
- Data size: 32 bits
- Frame length: 128 bits
- Active frame length: validate against DRDY pulse width
- 4 slots
- slot size: 32 bits
- active slots: 0,1,2,3
- MSB first
- PE4 = FS/DRDY
- PE5 = SCK/DCLK
- PE6 = SD/DOUT0
- DMA RX circular

The AD7768 is the data-interface master; STM32 SAI must not generate DCLK or FS.

## FORMAT0 hardware requirement
FORMAT0 must be logic 1 for four channels serialized on DOUT0. If FORMAT0=0,
DOUT0 only carries CH0 and the current 4-slot decoder is invalid.

## DMA / cache on STM32H743
The DMA buffer is tagged into `.sai_dma_buffer` and aligned to 32 bytes.
Create a linker section for it in DMA-accessible SRAM and mark that region non-cacheable with MPU.
Do not place it in DTCM.

Example linker concept:

```ld
.sai_dma_buffer (NOLOAD) :
{
  . = ALIGN(32);
  *(.sai_dma_buffer)
  . = ALIGN(32);
} >RAM_D2
```

## Runtime diagnostics
`board_adc_get_diagnostics()` exposes:
- frames decoded
- blocks produced/dropped
- channel-ID errors
- ERROR_FLAGGED count
- filter-not-settled count
- repeated-data count
- filter-saturated count
- DMA half/full callback counts
- measured sample rate in mHz
- device status register
- revision ID

Expected after filter settling:
- measured_rate_millihz ~= 1000000
- channel_id_errors = 0
- blocks_dropped = 0
- header_error_flagged = 0
- filter_saturated = 0

## First bench validation
1. Verify MCLK at AD7768 is 32.768 MHz with oscilloscope.
2. Verify FORMAT0 is high.
3. Check SPI register readback and revision ID.
4. Check Device Status has no CHIP_ERROR / NO_CLOCK_ERROR.
5. Probe DRDY: expected 1 kHz.
6. Probe DCLK and DOUT0.
7. Confirm each DRDY frame contains CH0, CH1, CH2, CH3 IDs in that order.
8. Run 10 minutes and confirm no dropped block and measured rate remains close to 1000 Hz.

## Important assumption
If the actual PCB MCLK is NOT 32.768 MHz, do not use this profile unchanged.
The driver deliberately rejects another MCLK/ODR pair rather than silently producing the wrong rate.

## ISR policy
SAI DMA half/full callbacks only enqueue a 1-byte event. TDM parsing and header validation run in `AcquisitionTask` context through `board_adc_get_block()`. This is intentional for STM32H7 determinism.

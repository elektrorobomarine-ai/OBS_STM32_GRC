# STM32H743 Pin Assignment

## 1. AD7768-4
| No | Function | AD7768-4 | STM32H743 | Peripheral |
|---:|---|---|---|---|
|1|SPI SCLK config|SCLK|PB10|SPI2_SCK|
|2|SPI MOSI config|SDI|PB15|SPI2_MOSI|
|3|SPI MISO readback|SDO|PB14|SPI2_MISO|
|4|SPI chip select|CS|PB4|GPIO output|
|5|ADC data output|DOUT0|PE6|SAI1_SD_A|
|6|ADC data clock|DCLK|PE5|SAI1_SCK_A|
|7|Data ready / frame|DRDY|PE4|SAI1_FS_A / frame|
|8|ADC reset|RESET|PD2|GPIO output|
|9|Start conversion|START|PA8|TIM1_CH1|
|10|Sync input|SYNC_IN|PD3|GPIO output|
|11|Sync output monitor|SYNC_OUT|PD4|GPIO input|

## 2. Ethernet RMII LAN8720
|Signal|STM32|Peripheral|
|---|---|---|
|REF_CLK|PA1|ETH_REF_CLK|
|MDIO|PA2|ETH_MDIO|
|MDC|PC1|ETH_MDC|
|CRS_DV|PA7|ETH_CRS_DV|
|RXD0|PC4|ETH_RXD0|
|RXD1|PC5|ETH_RXD1|
|TX_EN|PB11|ETH_TX_EN|
|TXD0|PB12|ETH_TXD0|
|TXD1|PB13|ETH_TXD1|

## 3. IMU UART
|Function|STM32|Peripheral|
|---|---|---|
|MCU TX -> IMU RX|PD8|USART3_TX|
|MCU RX <- IMU TX|PD9|USART3_RX|

## 4. Depth I2C
|Function|STM32|Peripheral|
|---|---|---|
|SCL|PB8|I2C1_SCL|
|SDA|PB9|I2C1_SDA|

## 5. Analog
|Function|STM32|Peripheral|
|---|---|---|
|Leak|PA0|ADC1_INP16|
|Battery monitor|PC0|ADC1_INP10|

## 6. GPIO
|Function|STM32|Mode|
|---|---|---|
|Relay 1|PC6|Output|
|Relay 2|PC7|Output|

## 7. USB Logger
Default proposal: USB OTG FS Host MSC.
|Function|STM32|Peripheral|
|---|---|---|
|D-|PA11|USB_OTG_FS_DM|
|D+|PA12|USB_OTG_FS_DP|
|VBUS sense|PA9|USB_OTG_FS_VBUS (recommended)|
|ID|PA10|USB_OTG_FS_ID (optional)|

For flash-disk logging, provide an external 5 V VBUS power switch.

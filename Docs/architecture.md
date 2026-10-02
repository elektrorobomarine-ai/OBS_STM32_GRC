# Firmware Architecture

```text
+---------------------------------------------------------+
|                    OBS Application                      |
|  Acquisition | Sensor | Control | Health | Logger       |
+-------------------------+-------------------------------+
                          |
+-------------------------v-------------------------------+
|                  OBS Middleware                         |
| Protocol | Network | UTC Time | USB BIN Logger          |
+-------------------------+-------------------------------+
                          |
+-------------------------v-------------------------------+
|                    Board Drivers                        |
| AD7768 | IMU | Depth | Analog | Relay                   |
+-------------------------+-------------------------------+
                          |
+-------------------------v-------------------------------+
|      STM32Cube HAL / FreeRTOS / LwIP / USB / FATFS      |
+---------------------------------------------------------+
```

## Dependency rule
- Board drivers may depend on HAL.
- Middleware may depend on board-independent types and STM32 time primitives.
- Application may depend on middleware and board drivers.
- A module must not directly manipulate another module's peripheral handle.

## Suggested command set between OBS Runtime and MCU
- GET_STATUS
- SET_TIME_UTC
- START_STREAM
- STOP_STREAM
- START_USB_LOG
- STOP_USB_LOG
- RELAY1_ON / RELAY1_OFF
- RELAY2_ON / RELAY2_OFF
- GET_SENSOR
- GET_VERSION
- REBOOT

Exact command framing should be frozen together with OBS Runtime.

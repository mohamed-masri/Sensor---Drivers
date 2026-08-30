# GPS NEO STM32 Driver

A STM32 HAL-based driver for NEO-series GPS modules using NMEA data.

The driver is primarily developed and tested using the STM32F767ZI microcontroller on the NUCLEO-F767ZI development board.

---

## Features

- UART GPS communication
- DMA-based UART reception
- UART IDLE-line detection
- Software ring buffer
- NMEA sentence processing
- NMEA checksum validation
- Latitude extraction
- Longitude extraction
- Altitude extraction
- GPS fix validation
- NMEA coordinate conversion
- Continuous GPS data reception
- Buffer overflow handling

---

## Hardware

### Microcontroller

- STM32F767ZI

### Development Board

- NUCLEO-F767ZI

### GPS Module

- NEO-series GPS module

---

## Communication Interface

The current implementation uses:

**UART + DMA + IDLE-line detection**

through STM32 HAL.

UART data is received using DMA and transferred into a software ring buffer.

The received data is processed outside the UART callback.

---

## Driver Architecture

```text
GPS Module
     |
     | UART
     v
UART DMA
     |
     | IDLE Detection
     v
DMA Buffer
     |
     v
Ring Buffer
     |
     v
GPS_Process()
     |
     v
NMEA Parser
     |
     v
GPS Data

# LoRa RA-01 STM32 Driver

A STM32 HAL-based driver for the RA-01 LoRa module based on the Semtech SX1278 LoRa transceiver.

The driver is primarily developed and tested using the STM32F767ZI microcontroller on the NUCLEO-F767ZI development board.

---

## Features

- SX1278 device initialization
- Hardware reset
- SPI register access
- LoRa mode configuration
- Frequency configuration
- Bandwidth configuration
- Coding rate configuration
- Spreading factor configuration
- CRC configuration
- Preamble configuration
- Sync word configuration
- TX power configuration
- Continuous receive mode
- Packet transmission
- Packet reception
- DIO0 interrupt handling
- Error status reporting
- Non-blocking transmission interface

---

## Hardware

### Microcontroller

- STM32F767ZI

### Development Board

- NUCLEO-F767ZI

### LoRa Module

- RA-01
- SX1278

---

## Communication Interface

The current driver uses:

**SPI**

through STM32 HAL for communication with the SX1278.

Low-level register and FIFO access currently uses HAL SPI polling/blocking transactions.

---

## Interrupt Interface

The RA-01 DIO0 pin is connected to an external interrupt.

DIO0 events are detected using STM32 EXTI.

The interrupt callback only signals the driver.

The actual radio event processing is performed outside the interrupt context using:

```c
RA01_Process();

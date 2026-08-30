# BMP388 STM32 Driver

A lightweight STM32 HAL-based driver for the Bosch BMP388 barometric pressure and temperature sensor.

The driver is primarily developed and tested using the STM32F767ZI microcontroller on the NUCLEO-F767ZI development board.

---

## Features

- BMP388 device initialization
- Chip ID verification
- Sensor reset
- Calibration data reading
- Temperature measurement
- Pressure measurement
- Altitude calculation
- Sensor configuration
- Error status reporting
- STM32 HAL integration

---

## Hardware

### Microcontroller

- STM32F767ZI

### Development Board

- NUCLEO-F767ZI

### Sensor

- Bosch BMP388

---

## Communication Interface

The current implementation communicates with the BMP388 using:

**I2C**

through the STM32 HAL library.

The current implementation uses blocking/polling I2C transactions.

---

## I2C Address

The default configured I2C address is:

```c
#define BMP388_I2C_ADDR (0x76 << 1)

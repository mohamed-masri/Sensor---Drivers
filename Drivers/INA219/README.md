# INA219 STM32 Driver

A STM32 HAL-based driver for the INA219 current, voltage, and power monitor.

The driver is primarily developed and tested using the STM32F767ZI microcontroller on the NUCLEO-F767ZI development board.

---

## Features

- INA219 initialization
- I2C communication
- Bus voltage measurement
- Shunt voltage measurement
- Current measurement
- Power measurement
- Sensor configuration
- STM32 HAL integration
- Error/status handling

---

## Hardware

### Microcontroller

- STM32F767ZI

### Development Board

- NUCLEO-F767ZI

### Sensor

- INA219

---

## Communication Interface

The current implementation uses:

**I2C**

through STM32 HAL.

The I2C communication mode should be configured according to the driver implementation.

---

## STM32CubeMX Configuration

Configure the required I2C peripheral using STM32CubeMX.

After generating the project:

1. Add the INA219 source files.
2. Add the INA219 header file.
3. Include the driver header.
4. Configure the I2C peripheral.
5. Initialize the INA219.
6. Read the required measurements.

---

## Dependencies

This driver requires:

- STM32 HAL
- I2C peripheral
- STM32F7 HAL
- Standard C headers used by the implementation

---

## Measurements

The driver can be used to obtain measurements such as:

- Bus voltage
- Shunt voltage
- Current
- Power

---

## Integration

Typical integration:

```text
STM32 Application
       |
       v
INA219 Driver
       |
       v
STM32 HAL I2C
       |
       v
INA219

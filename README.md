\# Rebels STM32 All Drivers



Reusable STM32 HAL-based drivers for sensors and communication modules.



These drivers are designed to be integrated into STM32 projects without requiring the complete example project.



\---



\## Hardware Platform



The drivers are primarily developed and tested using:



\- \*\*MCU:\*\* STM32F767ZI

\- \*\*Development Board:\*\* NUCLEO-F767ZI

\- \*\*Firmware Library:\*\* STM32 HAL

\- \*\*Configuration Tool:\*\* STM32CubeMX

\- \*\*IDE:\*\* Keil MDK-ARM / µVision



\---



\## Available Drivers



| Driver | Interface | Description |

|---|---|---|

| BMP388 | I2C | Temperature, pressure and altitude |

| GPS NEO | UART + DMA | GPS/NMEA data processing |

| LoRa RA-01 | SPI + EXTI | LoRa communication |

| LoRa E32 | UART | LoRa communication |

| INA219 | I2C | Voltage, current and power monitoring |



\---



\## Driver Philosophy



The purpose of this repository is to provide reusable drivers that can be easily integrated into STM32 projects.



Each driver is kept independent from the complete application project.



STM32CubeMX is used to configure the required peripherals, while STM32 HAL is used for hardware abstraction.



\---



\## Integration



To use a driver:



1\. Configure the required peripheral using STM32CubeMX.

2\. Add the required driver `.c` and `.h` files to your project.

3\. Include the driver header file.

4\. Initialize the driver.

5\. Use the driver's public API.



Each driver contains its own README with integration instructions and API information.



\---



\## Current Communication Interfaces



The communication interface depends on the driver.



\### BMP388



I2C communication using STM32 HAL.



The current implementation uses polling/blocking I2C transactions.



\### GPS NEO



UART reception using DMA and UART IDLE-line detection.



Received data is stored in a software buffer and processed by the application.



\### LoRa RA-01



SPI communication using STM32 HAL.



DIO0 is used with an EXTI interrupt for radio events.



\### LoRa E32



UART-based communication using STM32 HAL.



See the driver-specific README for implementation details.



\### INA219



I2C communication using STM32 HAL.



See the driver-specific README for implementation details.



\---



\## Maintenance and Future Improvements



The drivers are continuously maintained and improved.



Future updates may include:



\- Bug fixes

\- Performance improvements

\- API improvements

\- Better error handling

\- Improved portability

\- Interrupt-based communication

\- DMA-based communication

\- Non-blocking APIs

\- Additional configuration options

\- Additional examples and tests



The communication mode documented for each driver represents the currently implemented and tested version.



New communication modes and improvements may be added as development and testing continue.



\---



\## Development Environment



The drivers are developed using:



\- STM32CubeMX

\- STM32 HAL

\- Keil MDK-ARM / µVision

\- NUCLEO-F767ZI

\- STM32F767ZI



\---



\## Important Note



These drivers are primarily developed and tested on the STM32F767ZI using the NUCLEO-F767ZI development board.



When using a driver with another STM32 device or board, peripheral configuration, GPIO assignments, clock configuration, DMA settings, interrupt settings, and HAL configuration may need to be modified.



\---



\## Updates



This repository will be updated periodically as the drivers are improved, optimized, tested, and extended with new features.



Always check the latest version of the repository before integrating a driver into a new project.



\---



\## License



This repository is provided for embedded systems development, learning, experimentation, and testing.


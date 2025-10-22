# Arduino SPI DMA with CRC32

This project demonstrates how to use an STM32F446RE as an SPI master to read data from a XIAO RP2040 slave using DMA, calculate a CRC32 checksum, and then send the data back.

## Wiring Instructions

Connect the STM32F446RE to the XIAO RP2040 as follows:

| STM32F446RE | XIAO RP2040 |
|-------------|-------------|
| PA7 (MOSI)  | D7 (MOSI)   |
| PA6 (MISO)  | D6 (MISO)   |
| PA5 (SCK)   | D8 (SCK)    |
| PA4 (CS)    | D9 (CS)     |
| GND         | GND         |
| 5V          | 5V          |

## Setup and Usage

1.  **STM32F446RE (Master):**
    *   Open the `stm32f446re_master/stm32f446re_master.ino` sketch in the Arduino IDE.
    *   Select the correct board (Nucleo-F446RE) and port.
    *   Install the `STM32CRC` library from the Arduino Library Manager.
    *   Upload the sketch to the STM32F446RE.

2.  **XIAO RP2040 (Slave):**
    *   Open the `xiao_rp2040_slave/xiao_rp2040_slave.ino` sketch in the Arduino IDE.
    *   Select the "Seeed XIAO RP2040" board and the correct port.
    *   Upload the sketch to the XIAO RP2040.

3.  **Operation:**
    *   The STM32F446RE will initiate an SPI transaction, reading data from the XIAO RP2040 via DMA.
    *   After the DMA transfer is complete, the STM32F446RE will calculate a CRC32 checksum of the received data.
    *   The STM32F446RE will then send the original data followed by the 4-byte CRC32 checksum back to the XIAO RP2040.
    *   You can monitor the serial output of the STM32F446RE to see the process.

# Arduino SPI DMA with CRC32

This project demonstrates a robust SPI communication protocol between an STM32F446RE (Master) and a XIAO RP2040 (Slave). The master utilizes DMA for efficient data reception and its hardware CRC32 unit for offloading checksum calculations.

The process involves a two-phase transaction:
1.  The STM32 master reads a block of data from the XIAO slave using DMA.
2.  The STM32 calculates a CRC32 checksum of the received data using its hardware peripheral.
3.  The STM32 then transmits the original data, followed by the 4-byte CRC checksum, back to the XIAO slave for verification.

## Code Structure

-   `stm32f446re_master/`: Contains the Arduino sketch for the STM32F446RE master.
    -   `stm32f446re_master.ino`: Implements the master logic, including SPI/DMA initialization, transaction control, and hardware CRC calculation.
-   `xiao_rp2040_slave/`: Contains the Arduino sketch for the XIAO RP2040 slave.
    -   `xiao_rp2040_slave.ino`: Implements the slave logic, responding to the master's two-phase transaction.

## Wiring Instructions

Connect the STM32F446RE to the XIAO RP2040 as follows:

| STM32F446RE | XIAO RP2040 |
|:------------|:------------|
| `PA7` (MOSI)  | `D7` (MOSI)   |
| `PA6` (MISO)  | `D6` (MISO)   |
| `PA5` (SCK)   | `D8` (SCK)    |
| `PA4` (CS)    | `D9` (CS)     |
| `GND`         | `GND`         |
| `5V`          | `5V`          |

## Setup and Usage

1.  **STM32F446RE (Master):**
    *   Open the `stm32f446re_master/stm32f446re_master.ino` sketch in the Arduino IDE.
    *   Select the "Nucleo-F446RE" board from the Board Manager (`Tools > Board > STM32 Boards > Nucleo-64`).
    *   Install the `STM32CRC` library from the Arduino Library Manager.
    *   Compile and upload the sketch.

2.  **XIAO RP2040 (Slave):**
    *   Open the `xiao_rp2040_slave/xiao_rp2040_slave.ino` sketch in the Arduino IDE.
    *   Select the "Seeed XIAO RP2040" board (`Tools > Board > Arduino Mbed OS RP2040 Boards`).
    *   Compile and upload the sketch.

3.  **Operation:**
    *   Open the Serial Monitor for the STM32F446RE at `115200` baud.
    *   The master will initiate the transaction, receive data, calculate the CRC, and send the data plus CRC back to the slave.
    *   The slave will print the CRC it receives to its own Serial Monitor.

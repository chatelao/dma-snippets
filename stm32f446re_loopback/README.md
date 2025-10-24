# STM32F446RE SPI DMA Loopback Project

This project demonstrates a self-contained SPI loopback test on a single STM32F446RE board. It uses two SPI peripherals on the chip—one master and one slave—to perform a DMA-based data transfer. After data is sent from the master to the slave, the STM32's hardware CRC peripheral calculates a checksum of the received data to verify its integrity.

## Features

-   **SPI Master/Slave on a Single Chip:** Uses `SPI1` as the master and `SPI2` as the slave.
-   **DMA-based Transfer:** The slave receives data using a DMA channel to minimize CPU intervention.
-   **Hardware CRC32:** Utilizes the built-in CRC peripheral via the `STM32CRC` library to offload the checksum calculation from the CPU.
-   **Data Verification:** Compares the transmitted and received buffers byte-for-byte to ensure the integrity of the loopback transfer.

## Code Structure

-   `stm32f446re_loopback.ino`: The complete Arduino sketch for the test. It handles the initialization of both SPI peripherals, the DMA channel for the slave, the main loop that orchestrates the test, and the final data verification and CRC calculation.

## Wiring Instructions

To run this test, physically connect the SPI master pins to the SPI slave pins on your Nucleo-F446RE board.

| Master (SPI1) | Slave (SPI2)  | Connection          |
| :------------ | :------------ | :------------------ |
| `PA7` (MOSI)  | `PB15` (MOSI) | Connect `PA7` to `PB15` |
| `PA6` (MISO)  | `PB14` (MISO) | Connect `PA6` to `PB14` |
| `PA5` (SCK)   | `PB13` (SCK)  | Connect `PA5` to `PB13` |
| `PA4` (CS)    | `PB12` (CS)   | Connect `PA4` to `PB12` |

## Setup and Usage

1.  **Open in Arduino IDE:** Open the `stm32f446re_loopback.ino` sketch.
2.  **Install Board Support:** Ensure you have the "STM32 Cores" by STMicroelectronics board package installed from the Board Manager.
3.  **Install Libraries:** Install the `STM32CRC` library from the Arduino Library Manager.
4.  **Select Board:** Choose "Nucleo-F446RE" from the board selection menu.
5.  **Wire the Board:** Make the physical connections as described above.
6.  **Upload:** Compile and upload the sketch to your board.
7.  **Monitor Output:** Open the Serial Monitor at a baud rate of `115200`. You will see the status of each transfer, a data verification message, and the calculated CRC32 checksum. The test repeats every 2 seconds.

### Example Output

```
STM32F446RE SPI DMA Loopback Test

Starting SPI DMA transfer...
Transfer complete.
Data verification successful!
Hardware CRC32 of received data: 0x8919A965
```

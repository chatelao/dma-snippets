# STM32F446RE SPI DMA Loopback Project

This project demonstrates a self-contained SPI loopback test on an STM32F446RE. It uses two SPI peripherals on the same chip, one configured as a master and the other as a slave, to perform a DMA-based data transfer. After the data is transferred from the master to the slave, the STM32's hardware CRC peripheral is used to calculate a checksum of the received data.

## Features

-   **SPI Master/Slave on a single chip:** Uses SPI1 as the master and SPI2 as the slave.
-   **DMA-based Transfer:** The slave receives data using a DMA channel to minimize CPU intervention.
-   **Hardware CRC32:** Utilizes the built-in CRC peripheral to calculate a checksum, demonstrating how to offload this task from the CPU.
-   **Data Verification:** Compares the transmitted and received buffers to ensure the integrity of the loopback transfer.

## Wiring Instructions

To run this test, you need to physically connect the SPI master pins to the SPI slave pins on your Nucleo-F446RE board.

| Master (SPI1) | Slave (SPI2)  | Connection     |
| :------------ | :------------ | :------------- |
| `PA7` (MOSI)  | `PB15` (MOSI) | Connect PA7 to PB15 |
| `PA6` (MISO)  | `PB14` (MISO) | Connect PA6 to PB14 |
| `PA5` (SCK)   | `PB13` (SCK)  | Connect PA5 to PB13 |
| `PA4` (CS)    | `PB12` (CS)   | Connect PA4 to PB12 |

You will also need to power the board via USB.

## Setup and Usage

1.  **Open in Arduino IDE:** Open the `stm32f446re_loopback.ino` sketch in your Arduino IDE.
2.  **Install Board Support:** Ensure you have the "STM32 Cores" by STMicroelectronics board package installed. You can find it in the Board Manager.
3.  **Install Libraries:** Install the `STM32CRC` library from the Arduino Library Manager. This provides a user-friendly interface to the hardware CRC peripheral.
4.  **Select Board:** Choose "Nucleo-F446RE" from the board selection menu.
5.  **Wire the Board:** Make the physical connections as described in the "Wiring Instructions" section above.
6.  **Upload:** Compile and upload the sketch to your board.
7.  **Monitor Output:** Open the Serial Monitor at a baud rate of 115200. You should see output indicating the status of the SPI transfer, data verification, and the calculated CRC32 checksum. The test will repeat every 2 seconds.

### Example Output

```
STM32F446RE SPI DMA Loopback Test

Starting SPI DMA transfer...
Transfer complete.
Data verification successful!
Hardware CRC32 of received data: 0x8919A965

Starting SPI DMA transfer...
Transfer complete.
Data verification successful!
Hardware CRC32 of received data: 0x8919A965
```

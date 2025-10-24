# XIAO RP2040 SPI Dual-Core Loopback Project

This project demonstrates a self-contained, full-duplex SPI loopback test on a Seeed Studio XIAO RP2040. It showcases the multicore capabilities of the RP2040 by running two SPI peripherals simultaneously on separate cores:
-   **Core 0** runs the standard Arduino `loop()` function and acts as the **SPI Master** (using `SPI1`).
-   **Core 1** runs the `setup1()` and `loop1()` functions and acts as the **SPI Slave** (using `SPI0`).

The Arduino-Pico core's `SPI.transfer()` function utilizes DMA for transfers of this size, making this a DMA-based loopback. After the data is exchanged, a software-based CRC32 checksum is calculated to verify the integrity of the data received by the master.

## Features

-   **Dual-Core Operation:** A clear example of using `setup1()` and `loop1()` to run code on the RP2040's second core.
-   **SPI Master/Slave on a Single Chip:** Uses `SPI1` as the master and `SPI0` as the slave.
-   **Implicit DMA:** Leverages the built-in DMA capabilities of the underlying Arduino-Pico SPI library.
-   **Software CRC32:** Includes a standard CRC32 implementation to verify the received data without requiring hardware support.
-   **Full-Duplex Verification:** Checks the integrity of the data received by *both* the master and the slave.

## Code Structure

-   `xiao_rp2040_loopback.ino`: The complete Arduino sketch for the test. It contains the logic for both cores:
    -   `setup()` and `loop()`: Code for the master (Core 0).
    -   `setup1()` and `loop1()`: Code for the slave (Core 1).
    -   `software_crc32()`: A self-contained function for calculating the checksum.

## Wiring Instructions

Physically connect the master SPI pins to the slave SPI pins on your XIAO RP2040 board.

| Master (SPI1) Port | Master Pin | Slave (SPI0) Port | Slave Pin | Connection             |
| :----------------- | :--------- | :---------------- | :-------- | :--------------------- |
| `D1` (MOSI)        | `GPIO3`    | `D9` (MOSI)       | `GPIO7`   | Connect **D1 to D9**   |
| `D2` (MISO)        | `GPIO4`    | `D8` (MISO)       | `GPIO6`   | Connect **D2 to D8**   |
| `D0` (SCK)         | `GPIO2`    | `D6` (SCK)        | `GPIO8`   | Connect **D0 to D6**   |
| `D3` (CS)          | `GPIO5`    | `D7` (CS)         | `GPIO9`   | Connect **D3 to D7**   |

## Setup and Usage

1.  **Open in Arduino IDE:** Open the `xiao_rp2040_loopback.ino` sketch.
2.  **Install Board Support:** Ensure you have the "Arduino Mbed OS RP2040 Boards" package installed from the Board Manager.
3.  **Select Board:** Choose "Seeed XIAO RP2040" from the board selection menu (`Tools > Board > Arduino Mbed OS RP2040 Boards`).
4.  **Wire the Board:** Make the physical connections as described above.
5.  **Upload:** Compile and upload the sketch to your XIAO RP2040.
6.  **Monitor Output:** Open the Serial Monitor at a baud rate of `115200`. You will see the status of the transfer, a verification message for both master and slave, and the final software CRC32 checksum. The test repeats every 2 seconds.

### Example Output

```
XIAO RP2040 SPI DMA Loopback Test

Starting SPI DMA transfer...
Transfer complete.
Data verification successful on both master and slave!
Software CRC32 of data received by master: 0x1CB4926F
```

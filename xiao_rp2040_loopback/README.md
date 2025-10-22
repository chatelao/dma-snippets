# XIAO RP2040 SPI DMA Loopback Project

This project demonstrates a self-contained SPI loopback test on a Seeed Studio XIAO RP2040. It showcases the multicore capabilities of the RP2040 by running two SPI peripherals simultaneously:
-   **Core 0** runs the Arduino `loop()` function and acts as the **SPI Master** (using `SPI1`).
-   **Core 1** runs the Arduino `loop1()` function and acts as the **SPI Slave** (using `SPI0`).

The standard Arduino-Pico core's `SPI.transfer()` function utilizes DMA for transfers of a significant size, making this a DMA-based loopback. After the data is exchanged, a software-based CRC32 checksum is calculated to verify the integrity of the data received by the master.

## Features

-   **Dual-Core Operation:** A clear example of using `setup1()` and `loop1()` to run code on the RP2040's second core.
-   **SPI Master/Slave on a single chip:** Uses SPI1 as the master and SPI0 as the slave.
-   **DMA-based Transfer:** Leverages the built-in DMA capabilities of the underlying SPI library.
-   **Software CRC32:** Includes a simple, standard CRC32 implementation to verify the received data.
-   **Full-Duplex Verification:** Checks the integrity of the data received by both the master and the slave.

## Wiring Instructions

To run this test, you need to physically connect the master SPI pins to the slave SPI pins on your XIAO RP2040 board.

| Master (SPI1) Port | Master Pin | Slave (SPI0) Port | Slave Pin | Connection             |
| :----------------- | :--------- | :---------------- | :-------- | :--------------------- |
| `D1` (MOSI)        | `GPIO3`    | `D9` (MOSI)       | `GPIO7`   | Connect **D1 to D9**   |
| `D2` (MISO)        | `GPIO4`    | `D8` (MISO)       | `GPIO6`   | Connect **D2 to D8**   |
| `D0` (SCK)         | `GPIO2`    | `D6` (SCK)        | `GPIO8`   | Connect **D0 to D6**   |
| `D3` (CS)          | `GPIO5`    | `D7` (CS)         | `GPIO9`   | Connect **D3 to D7**   |

You will also need to power the XIAO RP2040 via its USB-C port.

## Setup and Usage

1.  **Open in Arduino IDE:** Open the `xiao_rp2040_loopback.ino` sketch in your Arduino IDE.
2.  **Install Board Support:** Ensure you have the "Arduino Mbed OS RP2040 Boards" package installed. In the Board Manager, search for "RP2040" and install the package by Arduino.
3.  **Select Board:** Choose "Seeed XIAO RP2040" from the board selection menu (`Tools > Board > Arduino Mbed OS RP2040 Boards`).
4.  **Wire the Board:** Make the physical connections as described in the "Wiring Instructions" section above.
5.  **Upload:** Compile and upload the sketch to your XIAO RP2040.
6.  **Monitor Output:** Open the Serial Monitor at a baud rate of 115200. You should see output indicating the status of the SPI transfer, data verification for both master and slave, and the calculated CRC32 checksum. The test will repeat every 2 seconds.

### Example Output

```
XIAO RP2040 SPI DMA Loopback Test

Starting SPI DMA transfer...
Transfer complete.
Data verification successful on both master and slave!
Software CRC32 of data received by master: 0x1CB4926F

Starting SPI DMA transfer...
Transfer complete.
Data verification successful on both master and slave!
Software CRC32 of data received by master: 0x1CB4926F
```

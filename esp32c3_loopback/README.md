# ESP32-C3 SPI DMA Loopback Test

This project demonstrates a full SPI master/slave loopback test on a single XIAO ESP32-C3 board. It uses two of the available SPI peripherals (HSPI and FSPI) to communicate with each other, leveraging DMA for efficient data transfer.

## How it Works

-   **HSPI (Master):** The HSPI peripheral is configured as the SPI master and initiates the data transfer.
-   **FSPI (Slave):** The FSPI peripheral is configured as the SPI slave and responds to the master.

The master and slave exchange a 256-byte buffer in a full-duplex transaction. After the transfer, the data is verified on both ends to ensure integrity. A software-based CRC32 checksum is also calculated on the data received by the master.

## Pinout

To run this test, you must wire the following pins on your XIAO ESP32-C3:

| Master (HSPI) | Slave (FSPI) |
| :------------ | :----------- |
| D1 (MOSI)     | D9 (MOSI)    |
| D2 (MISO)     | D8 (MISO)    |
| D0 (SCK)      | D6 (SCK)     |
| D3 (CS)       | D7 (CS)      |

## Compilation

To compile this sketch, you will need to add the Espressif board manager URL to your Arduino CLI configuration and install the `esp32:esp32` core. See the main `AGENTS.md` for more details.

Once configured, you can compile the sketch using the following command:

```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 esp32c3_loopback
```

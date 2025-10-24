# Arduino SPI Communication Projects

This repository contains a collection of Arduino projects demonstrating advanced SPI communication techniques, primarily between an STM32F446RE and a XIAO RP2040. The projects showcase features like DMA-based transfers, hardware and software CRC32 checksums, and dual-core programming on the RP2040.

## Projects

### 1. Master/Slave Communication (`arduino_spi_dma_crc`)

This is the main project, demonstrating a robust, two-phase communication protocol.
-   **`stm32f446re_master`**: An STM32F446RE master that receives data from a slave using DMA, calculates a CRC32 checksum with its hardware peripheral, and sends the data plus CRC back for verification.
-   **`xiao_rp2040_slave`**: A XIAO RP2040 slave that serves data to the master and receives the data and CRC back.

### 2. STM32 Loopback Test (`stm32f446re_loopback`)

A self-contained test on a single STM32F446RE board.
-   It uses two SPI peripherals on the same chip (`SPI1` as master, `SPI2` as slave) to perform a DMA-based loopback.
-   Verifies data integrity and calculates a hardware CRC32 checksum.
-   Ideal for testing the STM32's SPI, DMA, and CRC capabilities in isolation.

### 3. XIAO RP2040 Loopback Test (`xiao_rp2040_loopback`)

A self-contained test on a single XIAO RP2040 that highlights its dual-core architecture.
-   **Core 0** runs an SPI master.
-   **Core 1** runs an SPI slave.
-   Performs a full-duplex SPI loopback transfer.
-   Verifies data integrity and calculates a software-based CRC32 checksum.
-   Excellent for understanding multi-core programming with the Arduino framework.

## Getting Started

Each project folder contains a detailed `README.md` with specific wiring instructions, setup requirements, and usage examples. Please refer to the README within each directory to get started with that specific project.

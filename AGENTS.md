# AGENTS.md

This document provides instructions for AI agents working with this repository.

## Getting Started

### 1. Install Arduino CLI

Follow the official instructions to install the Arduino CLI for your operating system:
[https://arduino.github.io/arduino-cli/latest/installation/](https://arduino.github.io/arduino-cli/latest/installation/)

### 2. Initialize Configuration

Create the Arduino CLI configuration file.

```bash
arduino-cli config init
```

### 3. Add Board Manager URLs

Add the URLs for the STM32 and RP2040 board packages to your configuration.

```bash
arduino-cli config add board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
arduino-cli config add board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

### 4. Install Cores

Install the latest versions of the STM32, RP2040, and ESP32 cores.

```bash
arduino-cli core update-index
arduino-cli core install STMicroelectronics:stm32
arduino-cli core install rp2040:rp2040
arduino-cli core install esp32:esp32
```

## Compilation

Compile sketches using the following commands.

### STM32 Projects

-   **FQBN:** `STMicroelectronics:stm32:Nucleo_64:pnum=NUCLEO_F446RE`

```bash
# Main Master Project
arduino-cli compile --fqbn STMicroelectronics:stm32:Nucleo_64:pnum=NUCLEO_F446RE arduino_spi_dma_crc/stm32f446re_master

# Loopback Test
arduino-cli compile --fqbn STMicroelectronics:stm32:Nucleo_64:pnum=NUCLEO_F446RE stm32f446re_loopback
```

### RP2040 Projects

-   **FQBN:** `rp2040:rp2040:seeed_xiao_rp2040`

```bash
# Main Slave Project
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 arduino_spi_dma_crc/xiao_rp2040_slave

# Loopback Test
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 xiao_rp2040_loopback
```

### ESP32-C3 Projects

-   **FQBN:** `esp32:esp32:XIAO_ESP32C3`

```bash
# Loopback Test
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 esp32c3_loopback
```

## Coding Style

-   **Comments:** All functions, classes, and global variables must be documented using Doxygen-style comments. This is crucial for maintaining understandable and maintainable code.
-   **Formatting:** Adhere to the official Arduino Style Guide for consistency.
-   **Naming Conventions:**
    -   `camelCase` for variables and functions.
    -   `PascalCase` for structs and classes.
    -   `UPPER_SNAKE_CASE` for constants and macros.

## Testing

The repository includes self-contained loopback tests to verify the functionality of the hardware and code in isolation.

-   **STM32 Loopback (`stm32f446re_loopback`):** Compile and upload this sketch to an STM32F446RE board. The test uses two internal SPI peripherals to perform a loopback. Monitor the serial output for success or failure messages.
-   **RP2040 Loopback (`xiao_rp2040_loopback`):** Compile and upload this sketch to a XIAO RP2040. This test utilizes both cores to run a master and slave internally. Monitor the serial output for success or failure messages.
-   **ESP32-C3 Loopback (`esp32c3_loopback`):** Compile and upload this sketch to a XIAO ESP32-C3. This test uses two internal SPI peripherals (HSPI and FSPI) to perform a loopback. Monitor the serial output for success or failure messages.

Before submitting any changes, ensure that all relevant loopback tests pass.

## Commit Guidelines

Follow the Conventional Commits specification for all commit messages. This creates a more readable and structured commit history.

-   **Format:** `<type>(<scope>): <subject>`
-   **Example:** `feat(rp2040): add software crc32 calculation`

-   **Allowed Types:**
    -   `feat`: A new feature
    -   `fix`: A bug fix
    -   `docs`: Documentation only changes
    -   `style`: Changes that do not affect the meaning of the code (white-space, formatting, etc)
    -   `refactor`: A code change that neither fixes a bug nor adds a feature
    -   `perf`: A code change that improves performance
    -   `test`: Adding missing tests or correcting existing tests
    -   `ci`: Changes to our CI configuration files and scripts

## General Guidelines

-   **STM32 `Error_Handler`:** The function name `Error_Handler` is reserved by the STM32 core. Custom error handling functions must be named differently (e.g., `App_Error_Handler`) to avoid compilation conflicts.
-   **Hardware CRC on STM32:** The `STM32CRC` library is deprecated. Use the core-integrated hardware CRC functionality by using the `HAL_CRC_` functions directly.
-   **RP2040 SPISlave Library:** For SPI slave functionality on the RP2040, use the `SPISlave` library included with the `rp2040:rp2040` core.

/**
 * @file xiao_rp2040_slave.ino
 * @brief Full-Duplex SPI Slave on XIAO RP2040
 *
 * This sketch configures the XIAO RP2040 as a full-duplex SPI slave that
 * responds to a master (e.g., an STM32F446RE). The communication is handled
 * asynchronously using callbacks from the `SPISlave` library.
 *
 * The protocol operates as follows:
 * 1. The slave pre-loads a transmit buffer with data to be sent.
 * 2. When the master initiates a transaction, the `onDataReceive` callback is
 *    triggered.
 * 3. Inside the callback, the slave immediately re-queues the transmit buffer
 *    for the *next* transaction to ensure it's always ready.
 * 4. It then processes the just-received data, verifying its CRC32 checksum
 *    against a calculated value.
 *
 * This implementation uses the `SPISlave` library from the Earle Philhower
 * `rp2040:rp2040` core, which is non-blocking.
 */
#include <SPISlave.h>
#include <common_defs.h>

// --- Pin Definitions ---
#define SPI_MOSI_PIN 7      ///< SPI Master Out, Slave In (MOSI) pin for RP2040
#define SPI_MISO_PIN 6      ///< SPI Master In, Slave Out (MISO) pin for RP2040
#define SPI_SCK_PIN 8       ///< SPI Clock pin for RP2040
#define SPI_CS_PIN 9        ///< SPI Chip Select pin for RP2040

// --- Derived Buffer Sizes ---
#define RX_BUFFER_SIZE (BUFFER_SIZE + CRC_SIZE) ///< Total size for receiving data + CRC

// --- Buffers ---
/**
 * @brief Buffer for data to be sent to the master.
 *
 * This buffer is filled with sample data in `setup()`. In a full-duplex
 * transaction, its size must match the total transaction length.
 */
uint8_t txDataBuffer[BUFFER_SIZE + CRC_SIZE];

/**
 * @brief Buffer to store data and CRC received from the master.
 */
uint8_t rxCombinedBuffer[RX_BUFFER_SIZE];

/**
 * @brief Callback function executed when data is received from the SPI master.
 *
 * This function is the core of the slave's logic. It's triggered when the master
 * completes a transaction (specifically, Phase 2 of our protocol).
 *
 * @param data Pointer to the buffer containing the received data.
 * @param len The number of bytes received.
 */
void onDataReceive(uint8_t *data, size_t len) {
  // A transaction has just completed. First, re-queue the transmit buffer
  // for the *next* transaction so the slave is always ready.
  SPISlave.setData(txDataBuffer, sizeof(txDataBuffer));

  // Now, process the data we just received.
  if (len > 0 && len <= RX_BUFFER_SIZE) {
    memcpy(rxCombinedBuffer, data, len);

    // --- CRC Verification ---
    // Extract the CRC sent by the master (the last 4 bytes of the payload).
    uint32_t receivedCrc = *(uint32_t*)(rxCombinedBuffer + BUFFER_SIZE);

    // Calculate the CRC of the data portion of the buffer.
    uint32_t calculatedCrc = calculateCRC32(rxCombinedBuffer, BUFFER_SIZE);

    // Compare the calculated CRC with the received CRC.
    if (calculatedCrc == receivedCrc) {
      Serial.println("CRC verification successful!");
    } else {
      Serial.print("CRC verification FAILED! Calculated: 0x");
      Serial.print(calculatedCrc, HEX);
      Serial.print(", Received: 0x");
      Serial.println(receivedCrc, HEX);
    }
  }
}


/**
 * @brief Initializes the slave device.
 *
 * This function performs the following steps:
 * - Starts the Serial port for debugging.
 * - Fills the `txDataBuffer` with sample data (0 to 255).
 * - Configures the SPI pins for slave operation on the global `SPI` object.
 * - Sets up the `onDataReceive` callback.
 * - Pre-loads the initial data to be sent to the master for the first transaction.
 * - Initializes the SPI peripheral in slave mode.
 */
void setup() {
  Serial.begin(115200);

  // Fill the buffer with some data to be sent to the master
  for (int i = 0; i < BUFFER_SIZE; i++) {
    txDataBuffer[i] = i;
  }

  // Configure SPI pins for the Earle Philhower core.
  // Pin configuration is done on the SPI object (SPI, SPI1, etc.)
  SPI.setRX(SPI_MISO_PIN);
  SPI.setTX(SPI_MOSI_PIN);
  SPI.setSCK(SPI_SCK_PIN);
  SPI.setCS(SPI_CS_PIN);

  // Set the callback for when data is received from the master
  SPISlave.onDataRecv(onDataReceive);

  // Pre-load the data for the first transaction. In full-duplex, the slave must
  // be ready to send data of the full transaction length.
  SPISlave.setData(txDataBuffer, sizeof(txDataBuffer));

  // Initialize SPI slave. The SPISettings object is required but can be empty.
  SPISlave.begin(SPISettings());
}

/**
 * @brief Main execution loop for the slave.
 *
 * Since the `SPISlave` library is callback-driven, the main loop is empty.
 * All logic is handled by the `onDataReceive` function when an SPI
 * transaction occurs.
 */
void loop() {
  // Nothing to do here, all work is done in callbacks.
}

/**
 * @brief Calculates a standard CRC32 checksum compatible with the STM32 hardware.
 *
 * This function implements the standard CRC-32 algorithm (also known as
 * CRC-32/MPEG-2) which is compatible with the default configuration of the
 * STM32's hardware CRC peripheral.
 *
 * - Polynomial: 0x04C11DB7
 * - Initial Value: 0xFFFFFFFF
 * - Final XOR: 0x00000000
 * - No input or output reflection.
 *
 * @param data Pointer to the data buffer.
 * @param length The number of bytes in the buffer.
 * @return The calculated 32-bit CRC value.
 */
uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  const uint32_t polynomial = 0x04C11DB7;
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= (uint32_t)data[i] << 24;
    for (int j = 0; j < 8; j++) {
      if ((crc & 0x80000000) != 0) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

/**
 * @file xiao_rp2040_slave.ino
 * @brief SPI Slave on XIAO RP2040 using the SPISlave library.
 *
 * This sketch configures the XIAO RP2040 as an SPI slave device that responds
 * to a master (e.g., an STM32F446RE). The communication is a two-phase
 * transaction initiated and controlled by the master, handled asynchronously
 * using callbacks.
 *
 * 1.  **Phase 1 (Slave Send):** The master reads data from this slave. The slave
 *     pre-loads a data buffer to be sent when the master initiates a transaction.
 * 2.  **Phase 2 (Slave Receive):** The master sends data back to the slave,
 *     followed by a 4-byte CRC32 checksum. This is handled by a data receive
 *     callback, which then re-queues the data for the next Phase 1.
 *
 * This implementation uses the `SPISlave` library from the Earle Philhower
 * `rp2040:rp2040` core, which is non-blocking.
 */
#include <SPISlave.h>

// --- Pin and Buffer Definitions ---
#define SPI_MOSI_PIN 7      ///< SPI Master Out, Slave In (MOSI) pin for RP2040
#define SPI_MISO_PIN 6      ///< SPI Master In, Slave Out (MISO) pin for RP2040
#define SPI_SCK_PIN 8       ///< SPI Clock pin for RP2040
#define SPI_CS_PIN 9        ///< SPI Chip Select pin for RP2040
#define DATA_BUFFER_SIZE 256 ///< Size of the primary data payload, excluding CRC
#define CRC_SIZE 4           ///< Size of the CRC32 checksum in bytes
#define RX_BUFFER_SIZE (DATA_BUFFER_SIZE + CRC_SIZE) ///< Total size for receiving data + CRC

// --- Buffers ---
/**
 * @brief Buffer containing the data to be sent to the master in Phase 1.
 *
 * This buffer is filled with sample data in `setup()`.
 */
uint8_t txDataBuffer[DATA_BUFFER_SIZE];

/**
 * @brief Buffer to store the combined data and CRC received from the master in Phase 2.
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
  // Phase 2: Master has sent data + CRC.
  // Copy the received payload into our buffer.
  if (len > 0 && len <= RX_BUFFER_SIZE) {
    memcpy(rxCombinedBuffer, data, len);

    // For demonstration, print the received CRC.
    // The CRC is the last 4 bytes of the received buffer.
    uint32_t receivedCrc = *(uint32_t*)(rxCombinedBuffer + DATA_BUFFER_SIZE);
    Serial.print("Received data and CRC from master. CRC: 0x");
    Serial.println(receivedCrc, HEX);
  }

  // Important: Re-queue the data for the next time the master wants to read (Phase 1).
  // The SPISlave library needs its output buffer refilled after a transaction.
  SPISlave.setData(txDataBuffer, DATA_BUFFER_SIZE);
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
  for (int i = 0; i < DATA_BUFFER_SIZE; i++) {
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

  // Pre-load the data for the first Phase 1 transaction
  SPISlave.setData(txDataBuffer, DATA_BUFFER_SIZE);

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

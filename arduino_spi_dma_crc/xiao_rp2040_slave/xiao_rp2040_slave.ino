/**
 * @file xiao_rp2040_slave.ino
 * @brief SPI Slave on XIAO RP2040 for communication with an STM32 master.
 *
 * This sketch configures the XIAO RP2040 as an SPI slave device that responds
 * to a master (e.g., an STM32F446RE). The communication is a two-phase
 * transaction initiated and controlled by the master:
 *
 * 1.  **Phase 1 (Slave Send):** The master initiates a transfer to read data
 *     from this slave. The slave sends a pre-filled data buffer.
 * 2.  **Phase 2 (Slave Receive):** The master initiates a second transfer to
 *     send the data back to the slave, followed by a 4-byte CRC32 checksum.
 *     The slave receives this combined payload and can then perform data and
 *     CRC verification.
 *
 * The `SPI.transfer` method in the Arduino-Pico core is blocking and will wait
 * for the master to complete the transaction.
 */
#include <SPI.h>

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
 * @brief Buffer to receive the combined data and CRC from the master in Phase 2.
 *
 * This buffer is large enough to hold the original data payload plus the 4-byte CRC.
 */
uint8_t rxCombinedBuffer[RX_BUFFER_SIZE];

// --- SPI Instance ---
SPIClass slave; ///< SPI object configured to act as a slave.

/**
 * @brief Initializes the slave device.
 *
 * This function performs the following steps:
 * - Starts the Serial port for debugging.
 * - Fills the `txDataBuffer` with sample data (0 to 255).
 * - Configures the SPI pins for slave operation using the `SPIClass` object.
 * - Initializes the SPI peripheral in slave mode.
 */
void setup() {
  Serial.begin(115200);

  // Fill the buffer with some data to be sent to the master
  for (int i = 0; i < DATA_BUFFER_SIZE; i++) {
    txDataBuffer[i] = i;
  }

  // Configure SPI pins
  slave.setRX(SPI_MISO_PIN);
  slave.setTX(SPI_MOSI_PIN);
  slave.setSCK(SPI_SCK_PIN);
  slave.setCS(SPI_CS_PIN);

  // Initialize SPI as slave
  slave.begin(SPI_MODE_SLAVE);
}

/**
 * @brief Main execution loop for the slave.
 *
 * The slave's loop is synchronized with the master's actions. It consists of
 * two blocking `slave.transfer` calls that correspond to the two phases of
 * the master's transaction.
 *
 * 1.  **Phase 1:** The first `transfer` call sends the `txDataBuffer` to the
 *     master. The data received during this phase is ignored, as the primary
 *     purpose is for the master to read from the slave.
 * 2.  **Phase 2:** The second `transfer` call receives the combined data and CRC
 *     payload from the master into the `rxCombinedBuffer`. The data sent from
 *     the slave during this phase is irrelevant.
 *
 * After both phases are complete, the `rxCombinedBuffer` holds the data that can
 * be verified. For demonstration, this function prints the received CRC value
 * to the Serial monitor.
 */
void loop() {
  // This is a two-phase transaction controlled by the master.

  // Phase 1: Master reads data from slave.
  // We send the contents of txDataBuffer and ignore what we receive.
  slave.transfer(txDataBuffer, rxCombinedBuffer, DATA_BUFFER_SIZE);

  // Phase 2: Master sends data + CRC to slave.
  // We receive into rxCombinedBuffer. The content we send is irrelevant (dummy bytes from previous transaction).
  slave.transfer(rxCombinedBuffer, RX_BUFFER_SIZE);

  // At this point, rxCombinedBuffer contains the data and CRC sent by the master.
  // We can now verify the data and the CRC.

  // For demonstration, print the received CRC.
  // The CRC is the last 4 bytes of the received buffer.
  uint32_t receivedCrc = *(uint32_t*)(rxCombinedBuffer + DATA_BUFFER_SIZE);
  Serial.print("Received data and CRC from master. CRC: 0x");
  Serial.println(receivedCrc, HEX);

  // The master has a 1-second delay, so we'll be ready for the next transaction cycle.
}

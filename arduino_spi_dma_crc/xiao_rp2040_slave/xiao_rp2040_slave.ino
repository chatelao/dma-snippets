/**
 * @file xiao_rp2040_slave.ino
 * @brief SPI Slave on XIAO RP2040 using an event-driven, asynchronous API.
 *
 * This sketch configures the XIAO RP2040 to act as an SPI slave, responding
 * to a master device (e.g., an STM32F446RE). The communication protocol is
 * divided into two distinct phases, managed by a state machine:
 *
 * 1.  **Phase 1 (Provide Data):** When the master selects the slave, the slave
 *     populates its transmit buffer with sample data. This data is then sent
 *     to the master.
 *
 * 2.  **Phase 2 (Verify CRC):** On the next selection, the slave receives the
 *     original data back from the master, followed by a 4-byte CRC32 checksum.
 *     The slave calculates its own CRC32 of the data it received and compares
 *     it to the checksum sent by the master to verify data integrity.
 *
 * This implementation uses the modern, asynchronous, event-driven SPI API
 * available in the `rp2040:rp2040` core.
 */
#include <SPI.h>

// --- Pin Definitions ---
#define SPI_MOSI_PIN 8  ///< MOSI pin (Master Out, Slave In)
#define SPI_MISO_PIN 9  ///< MISO pin (Master In, Slave Out)
#define SPI_SCK_PIN 7   ///< SPI Clock pin
#define SPI_CS_PIN 6    ///< Chip Select pin

// --- Buffer and State Configuration ---
#define BUFFER_SIZE 256 ///< Size of the data buffer.

// Use the predefined SPI object for the slave
#define slave SPI

// Buffers for SPI communication
uint8_t tx_buffer[BUFFER_SIZE + sizeof(uint32_t)];
uint8_t rx_buffer[BUFFER_SIZE + sizeof(uint32_t)];

// State machine for two-phase transaction
enum TransactionState {
  STATE_PROVIDE_DATA,
  STATE_VERIFY_CRC
};
volatile TransactionState currentState = STATE_PROVIDE_DATA;

// --- Helper Functions ---
/**
 * @brief Calculates CRC32 for a given buffer.
 * @param data Pointer to the data buffer.
 * @param length Length of the data.
 * @return 32-bit CRC value.
 */
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }
  return ~crc;
}

// --- SPI Event Handlers ---
/**
 * @brief Called when the slave is selected by the master.
 *
 * This function prepares the transmit buffer based on the current state.
 */
void onSlaveSelect() {
  if (currentState == STATE_PROVIDE_DATA) {
    // Phase 1: Prepare sample data to be sent to the master.
    for (int i = 0; i < BUFFER_SIZE; i++) {
      tx_buffer[i] = (uint8_t)i;
    }
    slave.transfer(tx_buffer, nullptr, BUFFER_SIZE);
  } else {
    // Phase 2: Prepare to receive data and CRC from the master.
    slave.transfer(nullptr, rx_buffer, sizeof(rx_buffer));
  }
}

/**
 * @brief Called when the slave is deselected by the master.
 *
 * This function processes the received data and transitions the state machine.
 */
void onSlaveDeselect() {
  if (currentState == STATE_VERIFY_CRC) {
    // Phase 2 complete: Verify the CRC.
    uint32_t received_crc;
    memcpy(&received_crc, &rx_buffer[BUFFER_SIZE], sizeof(uint32_t));
    uint32_t calculated_crc = calculate_crc32(rx_buffer, BUFFER_SIZE);

    if (received_crc == calculated_crc) {
      Serial.println("CRC verification successful!");
    } else {
      Serial.println("CRC verification FAILED!");
    }
  }
  // Toggle state for the next transaction
  currentState = (currentState == STATE_PROVIDE_DATA) ? STATE_VERIFY_CRC : STATE_PROVIDE_DATA;
}

/**
 * @brief Called when data has been fully transferred.
 * @param rx_data Pointer to the received data.
 * @param tx_data Pointer to the transmitted data.
 * @param len Length of the transferred data.
 */
void onDataReceived(const uint8_t *rx_data, const uint8_t *tx_data, size_t len) {
  // This handler is required by the API but not needed for this logic,
  // as processing is done on deselect.
}


// --- Main Setup and Loop ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("XIAO RP2040 SPI Slave");

  // Configure SPI pins
  pinMode(SPI_MISO_PIN, OUTPUT);
  pinMode(SPI_MOSI_PIN, INPUT);
  pinMode(SPI_SCK_PIN, INPUT);
  pinMode(SPI_CS_PIN, INPUT);

  // Set up SPI event handlers
  slave.onSlaveSelect(onSlaveSelect);
  slave.onSlaveDeselect(onSlaveDeselect);
  slave.onDataReceived(onDataReceived);

  // Initialize SPI in slave mode
  slave.beginSlave(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, SPI_CS_PIN);
}

void loop() {
  // The event-driven API handles all SPI logic in the background.
  // The main loop can be used for other tasks.
  delay(100);
}

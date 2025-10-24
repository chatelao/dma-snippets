/**
 * @file xiao_rp2040_slave.ino
 * @brief SPI Slave on XIAO RP2040 using the SPISlave library.
 *
 * This sketch configures the XIAO RP2040 to act as an SPI slave, responding
 * to a master device (e.g., an STM32F446RE). The communication protocol is
 * divided into two distinct phases, managed by a state machine:
 *
 * 1.  **Phase 1 (Provide Data):** When the master initiates a transfer, the
 *     slave provides a buffer of sample data.
 *
 * 2.  **Phase 2 (Verify CRC):** On the next transfer, the slave receives the
 *     original data back from the master, followed by a 4-byte CRC32 checksum.
 *     It then verifies the integrity of this data.
 *
 * This implementation uses the SPISlave class from the Earle Philhower
 * RP2040 core, which relies on callbacks to handle data transmission and
 * reception.
 */
#include <SPISlave.h>

// --- Pin Definitions ---
#define SPI_MOSI_PIN 8
#define SPI_MISO_PIN 9
#define SPI_SCK_PIN 7
#define SPI_CS_PIN 6

// --- Buffer and State Configuration ---
#define BUFFER_SIZE 256
uint8_t tx_buffer[BUFFER_SIZE + sizeof(uint32_t)];
uint8_t rx_buffer[BUFFER_SIZE + sizeof(uint32_t)];
volatile size_t data_sent_len = 0;
volatile size_t data_recv_len = 0;

// State machine for two-phase transaction
enum TransactionState {
  STATE_PROVIDE_DATA,
  STATE_VERIFY_CRC
};
volatile TransactionState currentState = STATE_PROVIDE_DATA;

// --- Helper Functions ---
/**
 * @brief Calculates CRC32 for a given buffer.
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

// --- SPI Slave Callbacks ---
/**
 * @brief Provides data to be sent to the master.
 */
void dataSent() {
  if (currentState == STATE_PROVIDE_DATA) {
    // Phase 1: Provide sample data to the master
    if (data_sent_len < BUFFER_SIZE) {
      SPISlave.push(tx_buffer[data_sent_len++]);
    } else {
      SPISlave.push(0); // Push dummy byte if master requests more
    }
  }
}

/**
 * @brief Consumes data received from the master.
 */
void dataReceived(uint8_t data) {
  if (currentState == STATE_VERIFY_CRC) {
    // Phase 2: Receive data and CRC from the master
    if (data_recv_len < sizeof(rx_buffer)) {
      rx_buffer[data_recv_len++] = data;
    }
  }
}


// --- Main Setup and Loop ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("XIAO RP2040 SPI Slave");

  // Prepare initial data for Phase 1
  for (int i = 0; i < BUFFER_SIZE; i++) {
    tx_buffer[i] = (uint8_t)i;
  }

  // Set up SPISlave callbacks and begin operation
  SPISlave.onDataSent(dataSent);
  SPISlave.onDataRecv(dataReceived);
  SPISlave.begin(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, SPI_CS_PIN);
}

void loop() {
  // Check if a transaction has completed
  if (!digitalRead(SPI_CS_PIN)) {
    // Transfer in progress
  } else {
    // Transfer is complete, process the results
    if (currentState == STATE_PROVIDE_DATA && data_sent_len > 0) {
      // Transition to the next state after providing data
      currentState = STATE_VERIFY_CRC;
      data_sent_len = 0; // Reset for next transaction
    } else if (currentState == STATE_VERIFY_CRC && data_recv_len > 0) {
      // Verify CRC after receiving data
      uint32_t received_crc;
      memcpy(&received_crc, &rx_buffer[BUFFER_SIZE], sizeof(uint32_t));
      uint32_t calculated_crc = calculate_crc32(rx_buffer, BUFFER_SIZE);

      if (received_crc == calculated_crc) {
        Serial.println("CRC verification successful!");
      } else {
        Serial.println("CRC verification FAILED!");
      }
      // Transition back to the initial state
      currentState = STATE_PROVIDE_DATA;
      data_recv_len = 0; // Reset for next transaction
    }
  }
}

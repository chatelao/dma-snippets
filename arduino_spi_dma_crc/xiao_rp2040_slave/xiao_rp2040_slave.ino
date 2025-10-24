/**
 * @file xiao_rp2040_slave.ino
 * @brief SPI Slave on XIAO RP2040 using the correct SPISlave library API.
 *
 * This sketch configures the XIAO RP2040 to act as an SPI slave, responding
 * to a master device. The communication is divided into two phases managed by a
 * state machine:
 *
 * 1.  **Phase 1 (Provide Data):** The slave provides a buffer of sample data.
 * 2.  **Phase 2 (Verify CRC):** The slave receives the data back, plus a CRC,
 *     and verifies its integrity.
 *
 * This implementation uses the SPISlave class from the Earle Philhower
 * RP2040 core, which relies on callbacks with specific signatures to handle
 * data transmission and reception.
 */
#include <SPI.h>
#include <SPISlave.h>

// --- Pin Definitions ---
#define SPI_MOSI_PIN 8
#define SPI_MISO_PIN 9
#define SPI_SCK_PIN 7
#define SPI_CS_PIN 6

// --- Buffer and State Configuration ---
#define BUFFER_SIZE 256
uint8_t tx_buffer[BUFFER_SIZE];
uint8_t rx_buffer[BUFFER_SIZE + sizeof(uint32_t)];
volatile size_t tx_idx = 0;
volatile size_t rx_idx = 0;
volatile bool transfer_in_progress = false;

enum TransactionState {
  STATE_PROVIDE_DATA,
  STATE_VERIFY_CRC
};
volatile TransactionState currentState = STATE_PROVIDE_DATA;

// --- Helper Function for CRC ---
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
  }
  return ~crc;
}

// --- SPI Slave Callbacks (Correct Implementation) ---
/**
 * @brief Provides the next byte to be sent to the master.
 * @return The byte to transmit.
 */
uint8_t dataSent() {
  if (currentState == STATE_PROVIDE_DATA && tx_idx < BUFFER_SIZE) {
    return tx_buffer[tx_idx++];
  }
  // In CRC phase or if master reads too much, send dummy data.
  return 0;
}

/**
 * @brief Consumes a buffer of data received from the master.
 * @param data Pointer to the received data.
 * @param len The number of bytes received.
 */
void dataReceived(uint8_t *data, size_t len) {
  if (currentState == STATE_VERIFY_CRC) {
    size_t bytes_to_copy = min(len, sizeof(rx_buffer) - rx_idx);
    if (bytes_to_copy > 0) {
      memcpy(&rx_buffer[rx_idx], data, bytes_to_copy);
      rx_idx += bytes_to_copy;
    }
  }
}

// --- Main Setup and Loop ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("XIAO RP2040 SPI Slave - Correct API");

  // Prepare initial data for Phase 1
  for (int i = 0; i < BUFFER_SIZE; i++) {
    tx_buffer[i] = (uint8_t)i;
  }

  // Configure SPI pins on the global SPI object
  SPI.setRX(SPI_MOSI_PIN);
  SPI.setTX(SPI_MISO_PIN);
  SPI.setSCK(SPI_SCK_PIN);
  SPI.setCS(SPI_CS_PIN);

  // Set up SPISlave callbacks
  SPISlave.onDataRecv(dataReceived);
  SPISlave.onDataSent(dataSent);

  // Initialize SPISlave
  SPISlave.begin(SPISettings());
}

void loop() {
  // The logic now relies on polling the CS pin to manage state transitions.
  if (digitalRead(SPI_CS_PIN) == LOW && !transfer_in_progress) {
    // Master has selected the slave, begin transaction
    transfer_in_progress = true;
    if (currentState == STATE_PROVIDE_DATA) {
      tx_idx = 0;
    } else {
      rx_idx = 0;
    }
  } else if (digitalRead(SPI_CS_PIN) == HIGH && transfer_in_progress) {
    // Master has deselected the slave, end transaction
    transfer_in_progress = false;

    if (currentState == STATE_PROVIDE_DATA) {
      // Finished providing data, switch to next state
      currentState = STATE_VERIFY_CRC;
    } else if (currentState == STATE_VERIFY_CRC) {
      // Finished receiving data for CRC check
      if (rx_idx == sizeof(rx_buffer)) {
        uint32_t received_crc;
        memcpy(&received_crc, &rx_buffer[BUFFER_SIZE], sizeof(uint32_t));
        uint32_t calculated_crc = calculate_crc32(rx_buffer, BUFFER_SIZE);

        if (received_crc == calculated_crc) {
          Serial.println("CRC verification successful!");
        } else {
          Serial.println("CRC verification FAILED!");
        }
      }
      // Switch back to the initial state
      currentState = STATE_PROVIDE_DATA;
    }
  }
}

/**
 * @file xiao_rp2040_loopback.ino
 * @brief Self-contained SPI loopback test on a XIAO RP2040 using its dual cores
 *        and the correct SPISlave API.
 *
 * This sketch runs two separate sketches concurrently on the RP2040's two cores:
 *
 * - **Core 0** is configured as an **SPI Master**. It continuously transmits a
 *   block of sample data and then receives it back from the slave.
 *
 * - **Core 1** is configured as an **SPI Slave**, using the `SPISlave` library.
 *   It waits for the master to initiate a transfer, receives the data, and
 *   then echoes it back.
 *
 * This implementation uses the correct, callback-based API for the SPISlave
 * library from the Earle Philhower RP2040 core.
 */
#include <SPI.h>
#include <SPISlave.h>

// --- Pin Definitions for Loopback ---
#define SPI_MOSI_PIN 8
#define SPI_MISO_PIN 9
#define SPI_SCK_PIN 7
#define SPI_CS_PIN 6

// --- Buffer Configuration ---
#define BUFFER_SIZE 256
uint8_t master_tx_buffer[BUFFER_SIZE];
uint8_t master_rx_buffer[BUFFER_SIZE];

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


// =================================================================      //
// --- Core 1: SPI Slave Sketch ---                                       //
// =================================================================      //
#define SLAVE_BUFFER_SIZE 256
uint8_t slave_buffer[SLAVE_BUFFER_SIZE];
volatile size_t slave_tx_idx = 0;
volatile bool slave_should_send = false;

/**
 * @brief Callback for when the slave receives data. The data is immediately
 *        copied to the slave's transmit buffer to be echoed back.
 */
void slaveDataRecv(uint8_t *data, size_t len) {
  memcpy(slave_buffer, data, min(len, (size_t)SLAVE_BUFFER_SIZE));
  slave_tx_idx = 0;
  slave_should_send = true; // Flag that we are ready to send the echoed data
}

/**
 * @brief Callback for when the slave needs to send data.
 * @return The next byte to be transmitted.
 */
uint8_t slaveDataSent() {
  if (slave_should_send && slave_tx_idx < SLAVE_BUFFER_SIZE) {
    return slave_buffer[slave_tx_idx++];
  }
  // If we've sent everything or aren't ready, send a dummy byte
  if (slave_tx_idx >= SLAVE_BUFFER_SIZE) {
    slave_should_send = false;
  }
  return 0;
}

void setup1() {
  // Setup for Core 1 (Slave)
  SPI1.setRX(SPI_MOSI_PIN);
  SPI1.setTX(SPI_MISO_PIN);
  SPI1.setSCK(SPI_SCK_PIN);
  SPI1.setCS(SPI_CS_PIN);

  SPISlave1.onDataRecv(slaveDataRecv);
  SPISlave1.onDataSent(slaveDataSent);

  SPISlave1.begin(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, SPI_CS_PIN);
}

void loop1() {
  yield(); // Allow the core to sleep
}


// =================================================================      //
// --- Core 0: SPI Master Sketch ---                                      //
// =================================================================      //
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("XIAO RP2040 Dual-Core SPI Loopback Test - Correct API");

  // Fill the master transmit buffer with sample data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    master_tx_buffer[i] = (uint8_t)i;
  }

  // Use SPI for the master on the default pins
  SPI.setRX(SPI_MISO_PIN);
  SPI.setTX(SPI_MOSI_PIN);
  SPI.setSCK(SPI_SCK_PIN);
  SPI.setCS(SPI_CS_PIN);
  SPI.begin(false); // Master mode
}

void loop() {
  Serial.println("\nMaster: Starting transaction.");
  memset(master_rx_buffer, 0, BUFFER_SIZE);

  // Perform a full-duplex transfer
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SPI_CS_PIN, LOW);
  SPI.transfer(master_tx_buffer, master_rx_buffer, BUFFER_SIZE);
  digitalWrite(SPI_CS_PIN, HIGH);
  SPI.endTransaction();

  Serial.println("Master: Transaction complete.");

  // Verify the received data
  bool success = true;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (master_rx_buffer[i] != master_tx_buffer[i]) {
      Serial.print("Data mismatch at index ");
      Serial.println(i);
      success = false;
      break;
    }
  }

  if (success) {
    Serial.println("Master: Data verification successful!");
    uint32_t crc = calculate_crc32(master_rx_buffer, BUFFER_SIZE);
    Serial.print("Master: CRC32 of received data: 0x");
    Serial.println(crc, HEX);
  } else {
    Serial.println("Master: Data verification FAILED!");
  }

  delay(2000);
}

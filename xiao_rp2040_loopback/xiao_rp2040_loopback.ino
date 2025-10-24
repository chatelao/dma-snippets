/**
 * @file xiao_rp2040_loopback.ino
 * @brief Self-contained SPI loopback test on a XIAO RP2040 using its dual cores.
 *
 * This sketch demonstrates a powerful feature of the RP2040 by running two
 * separate sketches concurrently on its two cores, using the Earle Philhower
 * Arduino-Pico core.
 *
 * - **Core 0** is configured as an **SPI Master**. It continuously transmits a
 *   block of sample data and then receives it back from the slave.
 *
 * - **Core 1** is configured as an **SPI Slave**, using the `SPISlave` library.
 *   It waits for the master to initiate a transfer, receives the data, and
 *   then echoes it back.
 *
 * A software-based CRC32 calculation is performed on the master to verify the
 * integrity of the looped-back data. This example requires physical loopback
 * connections between the SPI pins.
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

// --- Helper Functions ---
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


// =================================================================      //
// --- Core 1: SPI Slave Sketch ---                                       //
// =================================================================      //
#define SLAVE_BUFFER_SIZE 256
uint8_t slave_tx_buffer[SLAVE_BUFFER_SIZE];
uint8_t slave_rx_buffer[SLAVE_BUFFER_SIZE];
volatile bool data_received = false;

// Callback for when the slave receives data
void slaveDataRecv(uint8_t data) {
  static size_t recv_count = 0;
  if (recv_count < SLAVE_BUFFER_SIZE) {
    slave_rx_buffer[recv_count++] = data;
  }
  if (recv_count >= SLAVE_BUFFER_SIZE) {
    memcpy(slave_tx_buffer, slave_rx_buffer, SLAVE_BUFFER_SIZE);
    data_received = true;
    recv_count = 0; // Reset for next transfer
  }
}

// Callback for when the slave needs to send data
void slaveDataSent() {
  static size_t sent_count = 0;
  if (data_received && sent_count < SLAVE_BUFFER_SIZE) {
    SPISlave.push(slave_tx_buffer[sent_count++]);
  } else {
    SPISlave.push(0); // Send dummy data if not ready
  }
  if (sent_count >= SLAVE_BUFFER_SIZE) {
    sent_count = 0;
    data_received = false;
  }
}

void setup1() {
  // Setup for Core 1 (Slave)
  SPISlave.onDataRecv(slaveDataRecv);
  SPISlave.onDataSent(slaveDataSent);
  SPISlave.begin(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, SPI_CS_PIN);
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
  Serial.println("XIAO RP2040 Dual-Core SPI Loopback Test");

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

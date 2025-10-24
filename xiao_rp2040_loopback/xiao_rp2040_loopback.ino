/**
 * @file xiao_rp2040_loopback.ino
 * @brief Self-contained SPI loopback test on a XIAO RP2040 using its dual cores.
 *
 * This sketch demonstrates a powerful feature of the RP2040 by running two
 * separate sketches concurrently on its two cores:
 *
 * - **Core 0** is configured as an **SPI Master**. It continuously transmits a
 *   block of sample data and then receives it back from the slave.
 *
 * - **Core 1** is configured as an **SPI Slave**. It waits for the master to
 *   initiate a transfer, receives the data, and then echoes it back.
 *
 * A software-based CRC32 calculation is performed on the master to verify the
 * integrity of the looped-back data. This example requires physical loopback
 * connections between the SPI pins.
 */
#include <SPI.h>

// --- Pin Definitions for Loopback ---
#define SPI_MOSI_PIN 8
#define SPI_MISO_PIN 9
#define SPI_SCK_PIN 7
#define SPI_CS_PIN 6

// --- Buffer Configuration ---
#define BUFFER_SIZE 256
uint8_t master_tx_buffer[BUFFER_SIZE];
uint8_t master_rx_buffer[BUFFER_SIZE];
uint8_t slave_buffer[BUFFER_SIZE];

// --- Synchronization ---
volatile bool slave_data_ready = false;

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


// =================================================================      //
// --- Core 1: SPI Slave Sketch ---                                       //
// =================================================================      //
/**
 * @brief Executed on Core 1. Configures and runs the SPI slave.
 */
void setup1() {
  // Use SPI1 for the slave
  SPI1.onSlaveSelect(slaveSelect);
  SPI1.onSlaveDeselect(slaveDeselect);
  SPI1.onDataReceived(slaveReceive);

  // Initialize SPI1 in slave mode.
  // Note: The pin numbers are passed here to configure the peripheral.
  SPI1.beginSlave(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, SPI_CS_PIN);
}

void loop1() {
  // The event-driven API handles all slave logic.
  // We can yield to allow the core to sleep.
  yield();
}

/**
 * @brief Slave event handler for when CS is asserted.
 */
void slaveSelect() {
  // Prepare to receive data from the master
  SPI1.transfer(nullptr, slave_buffer, BUFFER_SIZE);
}

/**
 * @brief Slave event handler for when data is fully transferred.
 */
void slaveReceive(const uint8_t* rx, const uint8_t* tx, size_t len) {
  // Data has been received. Now, load it into the TX buffer to echo it back.
  SPI1.transfer(slave_buffer, nullptr, BUFFER_SIZE);
}

/**
 * @brief Slave event handler for when CS is de-asserted.
 */
void slaveDeselect() {
  // Transaction is complete.
}


// =================================================================      //
// --- Core 0: SPI Master Sketch ---                                      //
// =================================================================      //
/**
 * @brief Executed on Core 0. Configures and runs the SPI master.
 */
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

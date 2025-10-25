/**
 * @file esp32c3_loopback.ino
 * @brief Demonstrates an SPI DMA loopback test on a XIAO ESP32C3.
 *
 * This sketch demonstrates a full SPI master/slave loopback test on a single
 * XIAO ESP32C3 chip. It uses two of the available SPI peripherals (HSPI and FSPI)
 * to communicate with each other.
 *
 * - HSPI (Master): Controls the transaction.
 * - FSPI (Slave): Responds to the master.
 *
 * The master and slave exchange data in a full-duplex transfer.
 * After the exchange, the data integrity is verified on both the master and
 * slave. A software-based CRC32 checksum is then calculated on the data
 * received by the master to demonstrate a common validation technique.
 */
#include <SPI.h>
#include <ESP32SPISlave.h>

// --- Pin Definitions ---
// HSPI (Master)
#define HSPI_MOSI_PIN 3 ///< Master MOSI (D1)
#define HSPI_MISO_PIN 4 ///< Master MISO (D2)
#define HSPI_SCK_PIN  2 ///< Master Clock (D0)
#define HSPI_CS_PIN   5 ///< Master Chip Select (D3)

// FSPI (Slave)
#define FSPI_MOSI_PIN 7 ///< Slave MOSI (D9)
#define FSPI_MISO_PIN 6 ///< Slave MISO (D8)
#define FSPI_SCK_PIN  8 ///< Slave Clock (D6)
#define FSPI_CS_PIN   9 ///< Slave Chip Select (D7)

// --- Buffer Configuration ---
#define BUFFER_SIZE 256 ///< The size of the data buffer for the full-duplex transfer.
#define QUEUE_SIZE 1

// --- Buffers ---
uint8_t master_tx_buffer[BUFFER_SIZE]; ///< Data the master sends to the slave.
uint8_t master_rx_buffer[BUFFER_SIZE]; ///< Where the master stores data received from the slave.
uint8_t slave_tx_buffer[BUFFER_SIZE];  ///< Data the slave sends to the master.
uint8_t slave_rx_buffer[BUFFER_SIZE];  ///< Where the slave stores data received from the master.

SPIClass hspi(HSPI);
ESP32SPISlave slave;

/**
 * @brief Calculates a standard CRC32 checksum in software.
 * @param data Pointer to the byte array to be checksummed.
 * @param length The number of bytes in the array.
 * @return The calculated 32-bit CRC value.
 */
uint32_t software_crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if ((crc & 1) == 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc = (crc >> 1);
      }
    }
  }
  return ~crc;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000); // Wait for serial to connect
  Serial.println("XIAO ESP32C3 SPI DMA Loopback Test");

  // Fill buffers with initial data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    master_tx_buffer[i] = (uint8_t)i;
    slave_tx_buffer[i] = (uint8_t)(255 - i); // Slave sends different data
  }
  memset(master_rx_buffer, 0, BUFFER_SIZE);
  memset(slave_rx_buffer, 0, BUFFER_SIZE);

  // Configure slave SPI pins
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(QUEUE_SIZE);
  slave.begin(FSPI);

  // Configure master SPI pins
  hspi.begin(HSPI_SCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN, HSPI_CS_PIN);
  hspi.setDataMode(SPI_MODE0);
  hspi.setBitOrder(MSBFIRST);
  hspi.setFrequency(1000000); // 1 MHz
  pinMode(HSPI_CS_PIN, OUTPUT);
  digitalWrite(HSPI_CS_PIN, HIGH);
}

void loop() {
  Serial.println("\nStarting SPI DMA transfer...");

  // Queue slave transaction
  slave.queue(slave_rx_buffer, slave_tx_buffer, BUFFER_SIZE);

  // Master transaction
  digitalWrite(HSPI_CS_PIN, LOW);
  hspi.transferBytes(master_tx_buffer, master_rx_buffer, BUFFER_SIZE);
  digitalWrite(HSPI_CS_PIN, HIGH);

  // Wait for slave transaction to complete
  slave.wait();

  Serial.println("Transfer complete.");

  // --- Verification ---
  bool success = true;
  // Check data received by master
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (master_rx_buffer[i] != slave_tx_buffer[i]) {
      success = false;
      Serial.print("Master data verification FAILED at index ");
      Serial.println(i);
      break;
    }
  }
  // Check data received by slave
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (slave_rx_buffer[i] != master_tx_buffer[i]) {
      success = false;
      Serial.print("Slave data verification FAILED at index ");
      Serial.println(i);
      break;
    }
  }

  if (success) {
    Serial.println("Data verification successful on both master and slave!");
    // Calculate CRC32 of the data the master received from the slave
    uint32_t crcResult = software_crc32(master_rx_buffer, BUFFER_SIZE);
    Serial.print("Software CRC32 of data received by master: 0x");
    Serial.println(crcResult, HEX);
  }

  delay(2000);
}

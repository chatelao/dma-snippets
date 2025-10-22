#include <SPI.h>

// This sketch demonstrates a self-contained SPI loopback test on a XIAO RP2040.
// It uses both of the RP2040's cores to run two SPI peripherals simultaneously:
// - SPI1 as Master (running on core 0)
// - SPI0 as Slave (running on core 1)
//
// The standard Arduino SPI library for the RP2040 uses DMA for transfers larger
// than a few bytes. After the loopback transfer, a software CRC32 is calculated.

// --- Pin Definitions ---
// Connect these pins together on your XIAO RP2040.
// Master (SPI1)
#define SPI1_MOSI_PIN 3 // D1
#define SPI1_MISO_PIN 4 // D2
#define SPI1_SCK_PIN  2 // D0
#define SPI1_CS_PIN   5 // D3

// Slave (SPI0)
#define SPI0_MOSI_PIN 7 // D9 (TX)
#define SPI0_MISO_PIN 6 // D8 (RX)
#define SPI0_SCK_PIN  8 // D6
#define SPI0_CS_PIN   9 // D7

// --- Buffer Configuration ---
#define BUFFER_SIZE 256

// --- SPI Instances ---
// SPI is for the default core 0 (Master)
// SPI_SLAVE is for core 1 (Slave)
SPIClass SPI_MASTER(spi1);
SPIClass SPI_SLAVE(spi0);

// --- Buffers ---
uint8_t master_tx_buffer[BUFFER_SIZE];
uint8_t master_rx_buffer[BUFFER_SIZE];
uint8_t slave_tx_buffer[BUFFER_SIZE];
uint8_t slave_rx_buffer[BUFFER_SIZE];

// --- Synchronization ---
volatile bool transferComplete = false;

// --- Software CRC32 Calculation ---
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

// --- Core 1 Setup and Loop (Slave) ---
void setup1() {
  // Configure slave SPI pins
  SPI_SLAVE.setRX(SPI0_MISO_PIN);
  SPI_SLAVE.setTX(SPI0_MOSI_PIN);
  SPI_SLAVE.setSCK(SPI0_SCK_PIN);
  SPI_SLAVE.setCS(SPI0_CS_PIN);

  // Begin SPI as slave
  SPI_SLAVE.begin(SPI_MODE_SLAVE);
}

void loop1() {
  // The slave will continuously handle transactions initiated by the master.
  // We prepare the transaction with the data it should send.
  SPI_SLAVE.transfer(slave_tx_buffer, slave_rx_buffer, BUFFER_SIZE);

  // After the master completes its transfer, this function will return.
  // We can then set the flag to notify the main core.
  transferComplete = true;
}

// --- Core 0 Setup and Loop (Master) ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000); // Wait for serial to connect
  Serial.println("XIAO RP2040 SPI DMA Loopback Test");

  // Fill buffers with initial data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    master_tx_buffer[i] = (uint8_t)i;
    slave_tx_buffer[i] = (uint8_t)(255 - i); // Slave sends different data
  }
  memset(master_rx_buffer, 0, BUFFER_SIZE);
  memset(slave_rx_buffer, 0, BUFFER_SIZE);

  // Configure master SPI pins
  SPI_MASTER.setTX(SPI1_MOSI_PIN);
  SPI_MASTER.setRX(SPI1_MISO_PIN);
  SPI_MASTER.setSCK(SPI1_SCK_PIN);
  pinMode(SPI1_CS_PIN, OUTPUT);
  digitalWrite(SPI1_CS_PIN, HIGH);

  // Begin SPI as master
  SPI_MASTER.begin(false); // Do not initialize bus, we already set pins
}

void loop() {
  Serial.println("\nStarting SPI DMA transfer...");
  transferComplete = false;

  // The slave core (loop1) is already running and waiting.
  // Now, the master initiates the transfer.
  digitalWrite(SPI1_CS_PIN, LOW);
  SPI_MASTER.transfer(master_tx_buffer, master_rx_buffer, BUFFER_SIZE);
  digitalWrite(SPI1_CS_PIN, HIGH);

  // Wait for the slave to confirm its side of the transfer is complete
  while(!transferComplete);

  Serial.println("Transfer complete.");

  // --- Verification ---
  bool success = true;
  // Check data received by master
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (master_rx_buffer[i] != slave_tx_buffer[i]) {
      success = false;
      Serial.println("Master data verification FAILED!");
      break;
    }
  }
  // Check data received by slave
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (slave_rx_buffer[i] != master_tx_buffer[i]) {
      success = false;
      Serial.println("Slave data verification FAILED!");
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

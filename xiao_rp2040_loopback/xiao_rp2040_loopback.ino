/**
 * @file xiao_rp2040_loopback.ino
 * @brief Demonstrates a dual-core SPI loopback test on a XIAO RP2040.
 *
 * This sketch provides a powerful demonstration of the RP2040's multicore
 * capabilities by running a full SPI master/slave loopback test on a single
 * chip. It leverages the Arduino-Pico core's support for the second core.
 *
 * - **Core 0 (Master):** Runs the standard `setup()` and `loop()` functions. It
 *   initializes and controls an SPI master (`SPI1`).
 * - **Core 1 (Slave):** Runs the `setup1()` and `loop1()` functions. It
 *   initializes and responds as an SPI slave (`SPI0`).
 *
 * The master and slave exchange data in a full-duplex transfer. The Arduino
 * core's SPI library automatically uses DMA for transfers of this size.
 * After the exchange, the data integrity is verified on both the master and
 * slave. A software-based CRC32 checksum is then calculated on the data
 * received by the master to demonstrate a common validation technique.
 */
#include <SPI.h>

// --- Pin Definitions ---
/**
 * @brief Physical pins that must be connected to create the loopback.
 * @note The XIAO RP2040 has multiple SERCOMs for SPI, so pins are carefully chosen.
 */
// Master (SPI1) - Core 0
#define SPI1_MOSI_PIN 3 ///< Master MOSI (D1)
#define SPI1_MISO_PIN 4 ///< Master MISO (D2)
#define SPI1_SCK_PIN  2 ///< Master Clock (D0)
#define SPI1_CS_PIN   5 ///< Master Chip Select (D3)

// Slave (SPI0) - Core 1
#define SPI0_MOSI_PIN 7 ///< Slave MOSI (D9)
#define SPI0_MISO_PIN 6 ///< Slave MISO (D8)
#define SPI0_SCK_PIN  8 ///< Slave Clock (D6)
#define SPI0_CS_PIN   9 ///< Slave Chip Select (D7)

// --- Buffer Configuration ---
#define BUFFER_SIZE 256 ///< The size of the data buffer for the full-duplex transfer.

// --- SPI Instances ---
SPIClass SPI_MASTER(spi1); ///< SPI instance for the master, running on Core 0.
SPIClass SPI_SLAVE(spi0);  ///< SPI instance for the slave, running on Core 1.

// --- Buffers ---
uint8_t master_tx_buffer[BUFFER_SIZE]; ///< Data the master sends to the slave.
uint8_t master_rx_buffer[BUFFER_SIZE]; ///< Where the master stores data received from the slave.
uint8_t slave_tx_buffer[BUFFER_SIZE];  ///< Data the slave sends to the master.
uint8_t slave_rx_buffer[BUFFER_SIZE];  ///< Where the slave stores data received from the master.

// --- Synchronization ---
/**
 * @brief A volatile flag used to synchronize the two cores after the transfer.
 *
 * Core 1 (slave) sets this to true after its `transfer` operation completes,
 * signaling to Core 0 (master) that the transaction is finished on both ends.
 */
volatile bool transferComplete = false;

/**
 * @brief Calculates a standard CRC32 checksum in software.
 * @param data Pointer to the byte array to be checksummed.
 * @param length The number of bytes in the array.
 * @return The calculated 32-bit CRC value.
 *
 * This function implements a bit-wise CRC32 algorithm, which is a common
 * method for verifying data integrity when a hardware CRC unit is not used.
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

/**
 * @brief Setup function for Core 1 (Slave).
 *
 * This function is automatically called by the Arduino-Pico core. It configures
 * the pins for the slave SPI peripheral (`SPI0`) and initializes it in slave mode.
 */
void setup1() {
  // Configure slave SPI pins
  SPI_SLAVE.setRX(SPI0_MISO_PIN);
  SPI_SLAVE.setTX(SPI0_MOSI_PIN);
  SPI_SLAVE.setSCK(SPI0_SCK_PIN);
  SPI_SLAVE.setCS(SPI0_CS_PIN);

  // Begin SPI as slave
  SPI_SLAVE.begin(SPI_MODE_SLAVE);
}

/**
 * @brief Main loop for Core 1 (Slave).
 *
 * This loop runs concurrently with `loop()` on Core 0. It consists of a single
 * blocking call to `SPI_SLAVE.transfer()`. This function prepares the slave
 * to exchange data and waits until the master initiates and completes the
 * transaction. Once the transfer is done, it sets the `transferComplete` flag
 * to notify the master core.
 */
void loop1() {
  // The slave will continuously handle transactions initiated by the master.
  // We prepare the transaction with the data it should send.
  SPI_SLAVE.transfer(slave_tx_buffer, slave_rx_buffer, BUFFER_SIZE);

  // After the master completes its transfer, this function will return.
  // We can then set the flag to notify the main core.
  transferComplete = true;
}

/**
 * @brief Setup function for Core 0 (Master).
 *
 * This is the standard Arduino setup function. It initializes the Serial port,
 * fills the transmit buffers with distinct sample data, and configures the
 * SPI master peripheral (`SPI1`).
 */
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

/**
 * @brief Main loop for Core 0 (Master).
 *
 * This loop orchestrates the entire test sequence:
 * 1.  Initiates the SPI transaction by asserting the CS line and calling
 *     `SPI_MASTER.transfer()`. Since the slave in `loop1` is already waiting,
 *     this begins the full-duplex data exchange.
 * 2.  Waits for the `transferComplete` flag to be set by the slave core,
 *     ensuring the transaction is finished on both ends.
 * 3.  Performs a full data verification by checking that the master received
 *     the slave's data correctly and vice-versa.
 * 4.  If verification succeeds, it calculates and prints the software CRC32
 *     of the data received by the master.
 * 5.  Waits for 2 seconds before repeating the test.
 */
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

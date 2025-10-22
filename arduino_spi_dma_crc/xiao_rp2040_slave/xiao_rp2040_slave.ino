#include <SPI.h>

// Define SPI pins and buffer size
#define SPI_MOSI_PIN 7
#define SPI_MISO_PIN 6
#define SPI_SCK_PIN 8
#define SPI_CS_PIN 9
#define DATA_BUFFER_SIZE 256
#define CRC_SIZE 4
#define RX_BUFFER_SIZE (DATA_BUFFER_SIZE + CRC_SIZE)

// Buffers
uint8_t txDataBuffer[DATA_BUFFER_SIZE];
uint8_t rxCombinedBuffer[RX_BUFFER_SIZE]; // To receive data + CRC

// SPI slave object
SPIClass slave;

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

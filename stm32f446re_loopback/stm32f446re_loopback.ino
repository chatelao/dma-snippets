#include <SPI.h>
#include <STM32CRC.h>

// This sketch demonstrates a self-contained SPI loopback test on an STM32F446RE.
// It uses two SPI peripherals:
// - SPI1 as Master
// - SPI2 as Slave
// DMA is used for the transfer. After receiving the data, the hardware CRC peripheral
// is used to calculate a checksum.

// --- Pin Definitions ---
// Connect these pins together to create the loopback.
// SPI1 (Master)
#define SPI1_MOSI_PIN PA7
#define SPI1_MISO_PIN PA6
#define SPI1_SCK_PIN  PA5
#define SPI1_CS_PIN   PA4

// SPI2 (Slave)
#define SPI2_MOSI_PIN PB15
#define SPI2_MISO_PIN PB14
#define SPI2_SCK_PIN  PB13
#define SPI2_CS_PIN   PB12

// --- Buffer Configuration ---
#define BUFFER_SIZE 256

// --- Global Handles ---
SPI_HandleTypeDef hspi1; // Master
SPI_HandleTypeDef hspi2; // Slave
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi2_rx;

// --- Buffers ---
uint8_t master_tx_buffer[BUFFER_SIZE];
uint8_t slave_rx_buffer[BUFFER_SIZE];

// --- CRC Instance ---
STM32CRC crc;

// --- Synchronization Flag ---
volatile bool transferComplete = false;

// --- Error Handler ---
void Error_Handler() {
  Serial.println("An error occurred. Halting.");
  while (1);
}

// --- Peripheral Initialization ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("STM32F446RE SPI DMA Loopback Test");

  // Fill the master transmit buffer with sample data
  for (int i = 0; i < BUFFER_SIZE; i++) {
    master_tx_buffer[i] = (uint8_t)i;
  }
  memset(slave_rx_buffer, 0, BUFFER_SIZE); // Clear slave buffer

  // --- Clock Configuration ---
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE();

  // --- Master (SPI1) Initialization ---
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();

  // --- Slave (SPI2) Initialization ---
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  // Other parameters must match the master
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK) Error_Handler();

  // --- DMA for Slave RX (SPI2) ---
  hdma_spi2_rx.Instance = DMA1_Stream3;
  hdma_spi2_rx.Init.Channel = DMA_CHANNEL_0;
  hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi2_rx.Init.Mode = DMA_NORMAL;
  hdma_spi2_rx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK) Error_Handler();
  __HAL_LINKDMA(&hspi2, hdmarx, hdma_spi2_rx);

  // --- Interrupt Configuration ---
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

  // --- Start CRC Peripheral ---
  crc.begin();
}

// --- Main Loop ---
void loop() {
  Serial.println("\nStarting SPI DMA transfer...");
  transferComplete = false;

  // 1. Start the slave to receive data with DMA
  if (HAL_SPI_Receive_DMA(&hspi2, slave_rx_buffer, BUFFER_SIZE) != HAL_OK) {
    Error_Handler();
  }

  // 2. Start the master to transmit the data
  // The HAL_SPI_Transmit function is blocking, but that's fine here.
  // We wait for the slave's DMA to finish.
  if (HAL_SPI_Transmit(&hspi1, master_tx_buffer, BUFFER_SIZE, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  // 3. Wait for the slave's DMA reception to complete
  while (!transferComplete);

  Serial.println("Transfer complete.");

  // 4. Verify data and calculate CRC
  bool success = true;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (slave_rx_buffer[i] != master_tx_buffer[i]) {
      Serial.print("Data mismatch at index ");
      Serial.println(i);
      success = false;
      break;
    }
  }

  if (success) {
    Serial.println("Data verification successful!");
    // Calculate CRC32 using the hardware peripheral
    crc.reset();
    crc.add(slave_rx_buffer, BUFFER_SIZE);
    uint32_t crcResult = crc.getCRC();
    Serial.print("Hardware CRC32 of received data: 0x");
    Serial.println(crcResult, HEX);
  } else {
    Serial.println("Data verification FAILED!");
  }

  // Wait before the next cycle
  delay(2000);
}

// --- Interrupt Service Routines ---
extern "C" void DMA1_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

// --- HAL Callbacks ---
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI2) {
    transferComplete = true;
  }
}

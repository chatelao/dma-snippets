#include <SPI.h>
#include <STM32CRC.h>

// Define SPI pins and buffer size
#define SPI_MOSI_PIN PA7
#define SPI_MISO_PIN PA6
#define SPI_SCK_PIN PA5
#define SPI_CS_PIN PA4
#define BUFFER_SIZE 256

// SPI and DMA handles
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;

// Buffers
uint8_t rxBuffer[BUFFER_SIZE];
uint8_t txBuffer[BUFFER_SIZE + sizeof(uint32_t)]; // Data + CRC

// CRC instance
STM32CRC crc;

// Transfer complete flag
volatile bool transferComplete = false;

void setup() {
  Serial.begin(115200);

  // Initialize SPI
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    Error_Handler();
  }

  // Initialize DMA for SPI RX
  __HAL_RCC_DMA2_CLK_ENABLE();
  hdma_spi1_rx.Instance = DMA2_Stream0;
  hdma_spi1_rx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi1_rx.Init.Mode = DMA_NORMAL;
  hdma_spi1_rx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_spi1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK) {
    Error_Handler();
  }
  __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);

  // Configure DMA interrupt
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  // Start CRC peripheral
  crc.begin();
}

void loop() {
  // Start SPI reception with DMA
  digitalWrite(SPI_CS_PIN, LOW);
  if (HAL_SPI_Receive_DMA(&hspi1, rxBuffer, BUFFER_SIZE) != HAL_OK) {
    Error_Handler();
  }

  // Wait for DMA transfer to complete
  while (!transferComplete) {
    // wait
  }
  transferComplete = false;
  digitalWrite(SPI_CS_PIN, HIGH);

  // Calculate CRC32
  crc.reset();
  crc.add(rxBuffer, BUFFER_SIZE);
  uint32_t crcResult = crc.getCRC();

  // Prepare data for transmission (data + CRC)
  memcpy(txBuffer, rxBuffer, BUFFER_SIZE);
  memcpy(txBuffer + BUFFER_SIZE, &crcResult, sizeof(crcResult));

  // Send the data with CRC
  digitalWrite(SPI_CS_PIN, LOW);
  if (HAL_SPI_Transmit(&hspi1, txBuffer, sizeof(txBuffer), HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }
  digitalWrite(SPI_CS_PIN, HIGH);

  delay(1000);
}

// DMA transfer complete callback
extern "C" void DMA2_Stream0_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI1) {
    transferComplete = true;
  }
}

void Error_Handler(void) {
  while (1) {
    // An error has occurred, stay in an infinite loop
  }
}

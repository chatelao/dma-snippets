/**
 * @file stm32f446re_master.ino
 * @brief SPI Master on STM32F446RE using DMA and Hardware CRC
 *
 * This sketch configures the STM32F446RE as an SPI master to communicate with a
 * slave device (e.g., a XIAO RP2040). It performs a two-phase transaction:
 * 1. Receives a block of data from the slave using DMA to minimize CPU overhead.
 * 2. Calculates a CRC32 checksum of the received data using the STM32's
 *    dedicated hardware CRC peripheral.
 * 3. Transmits the original data followed by the 4-byte CRC checksum back to the
 *    slave for verification.
 *
 * This example relies on the STM32 HAL libraries, which are abstracted by the
 * "STM32 Cores" Arduino package.
 */
#include <SPI.h>
#include <STM32CRC.h>

// Define SPI pins and buffer size
#define SPI_MOSI_PIN PA7 ///< SPI Master Out, Slave In (MOSI) pin
#define SPI_MISO_PIN PA6 ///< SPI Master In, Slave Out (MISO) pin
#define SPI_SCK_PIN PA5  ///< SPI Clock pin
#define SPI_CS_PIN PA4   ///< SPI Chip Select pin
#define BUFFER_SIZE 256  ///< Size of the data buffer to be exchanged

// SPI and DMA handles
SPI_HandleTypeDef hspi1;        ///< HAL handle for SPI1 peripheral
DMA_HandleTypeDef hdma_spi1_rx; ///< HAL handle for DMA stream linked to SPI1 RX

// Buffers
uint8_t rxBuffer[BUFFER_SIZE]; ///< Buffer to store data received from the slave
uint8_t txBuffer[BUFFER_SIZE + sizeof(uint32_t)]; // Data + CRC. Buffer for outgoing data

// CRC instance
STM32CRC crc; ///< Instance of the STM32CRC library to access the hardware CRC unit

// Transfer complete flag
volatile bool transferComplete = false; ///< Flag to indicate completion of the DMA transfer

/**
 * @brief Initializes hardware peripherals.
 *
 * This function performs the following initializations:
 * - Starts the Serial port for debugging.
 * - Configures SPI1 in Master mode.
 * - Configures DMA2, Stream 0 to handle SPI1 data reception.
 * - Links the DMA stream to the SPI peripheral.
 * - Enables DMA interrupts.
 * - Starts the hardware CRC peripheral.
 */
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

/**
 * @brief Main execution loop.
 *
 * This loop continuously performs the two-phase SPI transaction:
 * 1. **Reception Phase:**
 *    - Asserts the chip select line.
 *    - Starts a non-blocking SPI reception using DMA.
 *    - Waits for the `transferComplete` flag to be set by the DMA interrupt.
 *    - De-asserts the chip select line.
 * 2. **CRC and Transmission Phase:**
 *    - Calculates the CRC32 of the received data using the hardware unit.
 *    - Prepares a new buffer containing the original data followed by the CRC.
 *    - Asserts the chip select line.
 *    - Transmits the combined buffer back to the slave using a blocking call.
 *    - De-asserts the chip select line.
 *    - Waits for 1 second before starting the next transaction.
 */
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

/**
 * @brief Interrupt Service Routine for DMA2, Stream 0.
 *
 * This function is the entry point for the DMA transfer complete interrupt. It calls
 * the HAL's generic DMA IRQ handler, which will in turn call the appropriate
 * transfer completion callback (e.g., `HAL_SPI_RxCpltCallback`).
 */
extern "C" void DMA2_Stream0_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

/**
 * @brief HAL callback for SPI reception completion.
 * @param hspi Pointer to the SPI_HandleTypeDef structure.
 *
 * This function is called by the HAL library when the DMA transfer associated with
 * SPI reception is complete. It sets the `transferComplete` flag to true to
 * unblock the main loop.
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI1) {
    transferComplete = true;
  }
}

/**
 * @brief Error handler function.
 *
 * This function is called if any HAL operation returns an error. It enters an
 * infinite loop, effectively halting the program.
 */
void Error_Handler(void) {
  while (1) {
    // An error has occurred, stay in an infinite loop
  }
}

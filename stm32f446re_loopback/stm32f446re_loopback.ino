/**
 * @file stm32f446re_loopback.ino
 * @brief Self-contained SPI loopback test on an STM32F446RE using DMA and hardware CRC.
 *
 * This sketch demonstrates a robust method for testing SPI communication on a single
 * STM32F446RE chip. It configures two separate SPI peripherals:
 * - **SPI1** acts as the **Master**.
 * - **SPI2** acts as the **Slave**.
 *
 * The master transmits a block of data, which is received by the slave on the same
 * chip via a physical loopback connection. The slave's reception is handled by
 * a DMA channel to ensure the transfer is efficient and does not block the CPU.
 *
 * After the transfer is complete, the sketch verifies the integrity of the received
 * data by comparing it to the original transmitted data. It then uses the STM32's
 * hardware CRC peripheral to calculate a CRC32 checksum of the received data,
 * demonstrating a hardware-accelerated method for data validation.
 */
#include <SPI.h>
#include <STM32CRC.h>

// --- Pin Definitions ---
/**
 * @brief Physical pins that must be connected to create the loopback.
 */
// SPI1 (Master)
#define SPI1_MOSI_PIN PA7  ///< Master MOSI pin
#define SPI1_MISO_PIN PA6  ///< Master MISO pin
#define SPI1_SCK_PIN  PA5  ///< Master Clock pin
#define SPI1_CS_PIN   PA4  ///< Master Chip Select pin

// SPI2 (Slave)
#define SPI2_MOSI_PIN PB15 ///< Slave MOSI pin
#define SPI2_MISO_PIN PB14 ///< Slave MISO pin
#define SPI2_SCK_PIN  PB13 ///< Slave Clock pin
#define SPI2_CS_PIN   PB12 ///< Slave Chip Select pin

// --- Buffer Configuration ---
#define BUFFER_SIZE 256 ///< The size of the data buffer to be transferred.

// --- Global Handles ---
SPI_HandleTypeDef hspi1; ///< HAL handle for the master SPI peripheral (SPI1).
SPI_HandleTypeDef hspi2; ///< HAL handle for the slave SPI peripheral (SPI2).
DMA_HandleTypeDef hdma_spi1_tx; ///< HAL handle for the master TX DMA (not used in this example).
DMA_HandleTypeDef hdma_spi2_rx; ///< HAL handle for the slave RX DMA.

// --- Buffers ---
uint8_t master_tx_buffer[BUFFER_SIZE]; ///< Buffer for data the master will transmit.
uint8_t slave_rx_buffer[BUFFER_SIZE];  ///< Buffer where the slave stores received data.

// --- CRC Instance ---
STM32CRC crc; ///< Instance of the STM32CRC library for hardware CRC calculation.

// --- Synchronization Flag ---
volatile bool transferComplete = false; ///< Flag to signal completion of the slave's DMA reception.

/**
 * @brief Handles HAL errors by printing a message and halting execution.
 */
void Error_Handler() {
  Serial.println("An error occurred. Halting.");
  while (1);
}

/**
 * @brief Initializes all necessary peripherals for the loopback test.
 *
 * This function orchestrates the setup of the STM32 peripherals:
 * - Initializes the Serial port for output.
 * - Fills the master transmit buffer with sample data.
 * - Enables clocks for DMA1, SPI1, and SPI2.
 * - Configures and initializes SPI1 as the master.
 * - Configures and initializes SPI2 as the slave.
 * - Configures DMA1, Stream 3 to handle reception for SPI2.
 * - Links the DMA stream to the SPI2 peripheral.
 * - Enables the DMA interrupt to signal transfer completion.
 * - Initializes the hardware CRC peripheral.
 */
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

/**
 * @brief Main execution loop for the loopback test.
 *
 * This loop continuously performs the following sequence:
 * 1.  Starts the slave (SPI2) to receive data non-blockingly via DMA.
 * 2.  Starts the master (SPI1) to transmit data using a blocking call.
 * 3.  Waits for the `transferComplete` flag, which is set by the slave's DMA
 *     completion interrupt.
 * 4.  Verifies the integrity of the transfer by comparing the slave's receive
 *     buffer with the master's transmit buffer.
 * 5.  If verification is successful, it calculates and prints the hardware
 *     CRC32 checksum of the received data.
 * 6.  Waits for 2 seconds before repeating the test.
 */
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

/**
 * @brief Interrupt Service Routine for DMA1, Stream 3.
 *
 * This ISR is triggered when the DMA transfer to the slave's receive buffer is
 * complete. It calls the HAL's generic handler, which then invokes the
 * appropriate callback (`HAL_SPI_RxCpltCallback`).
 */
extern "C" void DMA1_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

/**
 * @brief HAL callback for SPI reception completion.
 * @param hspi Pointer to the SPI_HandleTypeDef of the completed transfer.
 *
 * This function is called by the HAL when the DMA transfer finishes. It checks if
 * the completed transfer belongs to the slave (SPI2) and, if so, sets the
 * `transferComplete` flag.
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI2) {
    transferComplete = true;
  }
}

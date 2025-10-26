/**
 * @file stm32f446re_master.ino
 * @brief Non-Blocking, Full-Duplex SPI Master on STM32F446RE
 *
 * This sketch configures the STM32F446RE as a non-blocking, full-duplex SPI
 * master. It uses a state machine and DMA for both transmission and reception
 * to ensure the main loop remains free.
 *
 * The protocol is as follows:
 * 1. The master initiates a full-duplex transfer, simultaneously sending a
 *    payload and receiving one from the slave.
 * 2. The payload sent by the master consists of the data it received in the
 *    *previous* transaction, followed by a hardware-calculated CRC32 of that data.
 * 3. The transaction is handled entirely by DMA, with a callback function
 *    signaling its completion.
 *
 * This design is efficient, as the CPU is only involved in initiating the
 * transfer and calculating the CRC, not in the byte-by-byte data exchange.
 */
#include <SPI.h>
#include <common_defs.h>

// Define SPI pins
#define SPI_MOSI_PIN PA7 ///< SPI Master Out, Slave In (MOSI) pin
#define SPI_MISO_PIN PA6 ///< SPI Master In, Slave Out (MISO) pin
#define SPI_SCK_PIN PA5  ///< SPI Clock pin
#define SPI_CS_PIN PA4   ///< SPI Chip Select pin

// --- State Machine ---
enum MasterState {
  STATE_IDLE,          ///< Waiting to start the next transaction
  STATE_TRANSFER_WAIT  ///< Waiting for the current DMA transaction to complete
};
volatile MasterState currentState = STATE_IDLE;

// --- Handles ---
SPI_HandleTypeDef hspi1;        ///< HAL handle for SPI1 peripheral
DMA_HandleTypeDef hdma_spi1_rx; ///< HAL handle for DMA stream linked to SPI1 RX
DMA_HandleTypeDef hdma_spi1_tx; ///< HAL handle for DMA stream linked to SPI1 TX
CRC_HandleTypeDef hCrc;         ///< HAL handle for the CRC peripheral

// --- Buffers ---
// Buffers are sized to hold the main data payload plus the 4-byte CRC.
uint8_t rxBuffer[BUFFER_SIZE + CRC_SIZE]; ///< Buffer for incoming data
uint8_t txBuffer[BUFFER_SIZE + CRC_SIZE]; ///< Buffer for outgoing data

// --- Transfer Timing ---
unsigned long lastTransferTime = 0;          ///< Time of the last completed transfer
const unsigned long transferInterval = 1000; ///< Interval between transfers (ms)

// Forward declaration for the error handler
void App_Error_Handler(void);

/**
 * @brief Initializes hardware peripherals.
 *
 * This function performs the following initializations:
 * - Starts the Serial port for debugging.
 * - Configures SPI1 in Master mode.
 * - Configures the hardware CRC peripheral.
 * - Configures DMA for both SPI1 RX (DMA2, Stream 0) and TX (DMA2, Stream 3).
 * - Links DMA streams to the SPI peripheral.
 * - Enables DMA interrupts.
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
    App_Error_Handler();
  }

  // Initialize CRC peripheral
  __HAL_RCC_CRC_CLK_ENABLE();
  hCrc.Instance = CRC;
  if (HAL_CRC_Init(&hCrc) != HAL_OK) {
    App_Error_Handler();
  }

  // --- DMA Setup ---
  __HAL_RCC_DMA2_CLK_ENABLE();

  // RX DMA Stream (DMA2_Stream0, Channel 3 for SPI1_RX)
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
    App_Error_Handler();
  }
  __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  // TX DMA Stream (DMA2_Stream3, Channel 3 for SPI1_TX)
  hdma_spi1_tx.Instance = DMA2_Stream3;
  hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi1_tx.Init.Mode = DMA_NORMAL;
  hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK) {
    App_Error_Handler();
  }
  __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

/**
 * @brief Main execution loop, implemented as a non-blocking state machine.
 *
 * This loop checks if the system is in an idle state and if enough time has
 * passed since the last transaction. If both are true, it prepares and
 * initiates the next full-duplex SPI transaction using DMA. The loop itself
 * does not block, allowing other tasks to run.
 */
void loop() {
  if (currentState == STATE_IDLE && (millis() - lastTransferTime >= transferInterval)) {
    // --- Prepare Buffer for Next Transmission ---
    // The protocol requires sending back the data received in the *previous*
    // transaction, appended with a fresh CRC. For the first transfer, rxBuffer
    // is uninitialized, and we send that with a CRC, which is acceptable.

    // Calculate CRC on the previously received data payload.
    uint32_t crcResult = HAL_CRC_Calculate(&hCrc, (uint32_t *)rxBuffer, BUFFER_SIZE / 4);

    // Prepare the transmit buffer: previous data + new CRC.
    memcpy(txBuffer, rxBuffer, BUFFER_SIZE);
    memcpy(txBuffer + BUFFER_SIZE, &crcResult, sizeof(crcResult));

    // --- Start DMA Transfer ---
    currentState = STATE_TRANSFER_WAIT;
    digitalWrite(SPI_CS_PIN, LOW);

    // Start a full-duplex (Transmit and Receive) transaction.
    // The HAL library handles both DMA streams automatically.
    if (HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE + CRC_SIZE) != HAL_OK) {
      App_Error_Handler();
    }
  }
}

/**
 * @brief Interrupt Service Routine for DMA2, Stream 0 (SPI1 RX).
 */
extern "C" void DMA2_Stream0_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

/**
 * @brief Interrupt Service Routine for DMA2, Stream 3 (SPI1 TX).
 */
extern "C" void DMA2_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

/**
 * @brief HAL callback for SPI transmit/receive completion.
 * @param hspi Pointer to the SPI_HandleTypeDef structure.
 *
 * This function is called by the HAL when the full-duplex DMA transfer is
 * complete. It de-asserts the chip select line, records the completion time,
 * and sets the state machine back to IDLE.
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI1) {
    digitalWrite(SPI_CS_PIN, HIGH);
    lastTransferTime = millis();
    currentState = STATE_IDLE;
  }
}

/**
 * @brief Error handler function.
 */
void App_Error_Handler(void) {
  while (1) {
    // An error has occurred, stay in an infinite loop
  }
}

/**
 * @file arduino_adc_ring_buffer.ino
 * @brief Demonstrates continuous ADC reading into a ring buffer on a Seeed XIAO RP2040.
 *
 * This example continuously reads an analog value from an ADC pin, stores it in a
 * ring buffer, and prints the contents of the buffer to the serial monitor
 * every time the buffer is filled.
 */

//======================================================================================================================
//================================================= MACROS/CONSTANTS ===================================================
//======================================================================================================================

/**
 * @brief The size of the ring buffer.
 * @details This determines how many ADC samples are stored before they are printed.
 */
#define RING_BUFFER_SIZE 256

/**
 * @brief The ADC pin to read from.
 * @details On the XIAO RP2040, this corresponds to pin A0.
 */
#define ADC_PIN 26

/**
 * @brief The baud rate for serial communication.
 */
#define SERIAL_BAUD_RATE 115200

//======================================================================================================================
//=================================================== DATA STRUCTURES ==================================================
//======================================================================================================================

/**
 * @brief A structure to represent the ring buffer.
 */
typedef struct
{
    uint16_t buffer[RING_BUFFER_SIZE]; ///< The underlying data buffer.
    volatile uint16_t head;            ///< The index where the next item will be written.
    volatile uint16_t tail;            ///< The index of the oldest item to be read.
} RingBuffer;

//======================================================================================================================
//================================================= GLOBAL VARIABLES ===================================================
//======================================================================================================================

/**
 * @brief The global ring buffer instance.
 */
static RingBuffer gRingBuffer;

//======================================================================================================================
//=============================================== FUNCTION PROTOTYPES ==================================================
//======================================================================================================================

/**
 * @brief Initializes the ring buffer.
 * @param rb Pointer to the RingBuffer instance to initialize.
 */
void ringBufferInit(RingBuffer* rb);

/**
 * @brief Writes a value to the ring buffer.
 * @param rb Pointer to the RingBuffer instance.
 * @param value The 16-bit value to write.
 * @return true if the write was successful, false if the buffer is full.
 */
bool ringBufferWrite(RingBuffer* rb, uint16_t value);

/**
 * @brief Reads a value from the ring buffer.
 * @param rb Pointer to the RingBuffer instance.
 * @param value Pointer to a variable where the read value will be stored.
 * @return true if the read was successful, false if the buffer is empty.
 */
bool ringBufferRead(RingBuffer* rb, uint16_t* value);

/**
 * @brief Returns the number of items currently in the buffer.
 * @param rb Pointer to the RingBuffer instance.
 * @return The number of items.
 */
uint16_t ringBufferCount(const RingBuffer* rb);

//======================================================================================================================
//================================================= ARDUINO FRAMEWORK ==================================================
//======================================================================================================================

/**
 * @brief Arduino setup function.
 * @details Initializes serial communication, the ADC, and the ring buffer.
 */
void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial)
    {
        ; // Wait for the serial port to connect. Needed for native USB only.
    }

    analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
    pinMode(ADC_PIN, INPUT);

    ringBufferInit(&gRingBuffer);

    Serial.println("ADC Ring Buffer Example Initialized");
}

/**
 * @brief Arduino loop function.
 * @details Continuously reads the ADC and writes the value to the ring buffer.
 *          When the buffer is full, it prints the contents, clears the buffer,
 *          and then adds the latest reading.
 */
void loop()
{
    // Read the ADC value
    uint16_t adcValue = analogRead(ADC_PIN);

    // Try to write the value to the ring buffer. If it's full,
    // the write will fail.
    if (!ringBufferWrite(&gRingBuffer, adcValue))
    {
        // The buffer is full, so we print its contents.
        Serial.println("Buffer is full. Printing contents:");
        uint16_t value;
        while (ringBufferRead(&gRingBuffer, &value))
        {
            Serial.print(value);
            Serial.print(" ");
        }
        Serial.println("\n");
        delay(1000); // Pause for a second before filling the buffer again

        // Now that the buffer is empty, write the value that we couldn't
        // add earlier.
        ringBufferWrite(&gRingBuffer, adcValue);
    }
}

//======================================================================================================================
//=============================================== FUNCTION DEFINITIONS =================================================
//======================================================================================================================

void ringBufferInit(RingBuffer* rb)
{
    rb->head = 0;
    rb->tail = 0;
}

bool ringBufferWrite(RingBuffer* rb, uint16_t value)
{
    uint16_t nextHead = (rb->head + 1) % RING_BUFFER_SIZE;

    // Check if the buffer is full
    if (nextHead == rb->tail)
    {
        return false; // Buffer is full
    }

    rb->buffer[rb->head] = value;
    rb->head = nextHead;
    return true;
}

bool ringBufferRead(RingBuffer* rb, uint16_t* value)
{
    // Check if the buffer is empty
    if (rb->head == rb->tail)
    {
        return false; // Buffer is empty
    }

    *value = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    return true;
}

uint16_t ringBufferCount(const RingBuffer* rb)
{
    if (rb->head >= rb->tail)
    {
        return rb->head - rb->tail;
    }
    return RING_BUFFER_SIZE - (rb->tail - rb->head);
}

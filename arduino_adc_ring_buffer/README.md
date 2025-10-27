# ADC Ring Buffer Example

This example demonstrates how to continuously read an analog value from an ADC pin into a ring buffer using the Arduino framework on a Seeed XIAO RP2040.

## Description

The firmware performs the following actions:

1.  **Initializes Serial Communication:** Sets up the serial port for printing output.
2.  **Configures ADC:** Sets the ADC resolution to 12 bits.
3.  **Initializes Ring Buffer:** Prepares a ring buffer to store the ADC samples.
4.  **Continuously Reads ADC:** In the main loop, the firmware reads the value from the ADC.
5.  **Stores Data:** The ADC value is written to the ring buffer.
6.  **Prints Buffer:** When the buffer is full, its entire contents are printed to the serial monitor, and the process repeats.

## Hardware Requirements

*   **Board:** Seeed XIAO RP2040
*   **Components:** A variable voltage source (e.g., a potentiometer) connected to pin `A0`.

## How to Compile

Use the `arduino-cli` to compile the sketch. The FQBN for the XIAO RP2040 is `rp2040:rp2040:seeed_xiao_rp2040`.

```bash
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 arduino_adc_ring_buffer
```

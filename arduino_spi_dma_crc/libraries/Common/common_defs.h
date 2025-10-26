#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

/**
 * @file common_defs.h
 * @brief Shared definitions for the SPI master and slave projects.
 *
 * This file contains common constants and definitions used by both the STM32
 * master and the RP2040 slave to ensure consistency and simplify maintenance.
 */

/**
 * @def BUFFER_SIZE
 * @brief The size of the data buffer to be exchanged between master and slave.
 *
 * This defines the number of bytes in the primary data payload, excluding any
 * metadata like a CRC checksum. It must be a multiple of 4 for compatibility
 * with the 32-bit hardware CRC calculation on the STM32.
 */
#define BUFFER_SIZE 256

/**
 * @def CRC_SIZE
 * @brief The size of the CRC32 checksum in bytes.
 */
#define CRC_SIZE 4

#endif // COMMON_DEFS_H

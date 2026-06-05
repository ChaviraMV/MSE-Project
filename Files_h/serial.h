/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Carlos Villarreal and CETYS Universidad are not liable for any
 * misuse of this material.
 *
 *****************************************************************************/
/**
 * @file serial.h
 * @brief Serial Module - High-level API for terminal communication.
 *
 * The Serial module wraps the UART driver and the Utils module to provide
 * a convenient formatted-print interface for debugging and telemetry.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date    05/06/2026
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/

/** Maximum length of the internal formatting buffer */
#define SERIAL_BUFFER_SIZE  128U

/*** Type Prototypes ***/

/**
 * @brief Return status codes for the Serial module.
 */
typedef enum {
    SERIAL_OK      = 0,  /**< Operation successful  */
    SERIAL_ERROR   = 1,  /**< Generic error         */
    SERIAL_INVALID = 2   /**< Invalid parameter     */
} serial_status_t;

/*** Function Prototypes ***/

/**
 * @brief  Initializes the Serial module.
 *
 * Calls uart_init() internally to configure USART2 at 115200 baud.
 *
 * @return SERIAL_OK on success.
 */
serial_status_t serial_init(void);

/**
 * @brief  Sends a formatted string over UART.
 *
 * Formats the string using utils_snprintf() into an internal buffer,
 * then transmits each character with uart_write().
 *
 * Supports the same format specifiers as utils_snprintf:
 *   %d  - signed decimal integer
 *   %u  - unsigned decimal integer
 *   %x  - hexadecimal integer
 *   %s  - string
 *   %c  - single character
 *   %%  - literal percent sign
 *
 * @param  format  printf-style format string.
 * @param  ...     Variable arguments matching the format specifiers.
 * @return SERIAL_OK on success, SERIAL_INVALID if format is NULL.
 */
serial_status_t serial_printf(const char *format, ...);

#endif /* __SERIAL_H__ */

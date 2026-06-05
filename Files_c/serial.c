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
 * @file serial.c
 * @brief Serial Module Implementation
 *
 * Provides serial_init() and serial_printf() built on top of the UART
 * driver and the Utils formatting library.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date    05/06/2026
 */

/*** Includes ***/
#include "serial.h"
#include "uart.h"
#include "utils.h"

/*** Local Variables ***/

/** Internal character buffer used by serial_printf */
static char serial_buffer[SERIAL_BUFFER_SIZE];

/*** Function Definitions ***/

/**
 * @brief  Initializes the Serial module by initializing the UART driver.
 * @return SERIAL_OK on success.
 */
serial_status_t serial_init(void)
{
    uart_init();
    return SERIAL_OK;
}

/**
 * @brief  Sends a formatted string over UART.
 *
 * Uses utils_snprintf() to format the string into the internal buffer,
 * then iterates through the buffer sending each byte with uart_write()
 * until the null terminator is reached.
 *
 * @param  format  printf-style format string.
 * @param  ...     Variable arguments.
 * @return SERIAL_OK on success, SERIAL_INVALID if format is NULL.
 */
serial_status_t serial_printf(const char *format, ...)
{
    if (format == (const char *)0)
    {
        return SERIAL_INVALID;
    }

    /* Build the formatted string into the local buffer using utils_snprintf.
     * We need to forward the variadic arguments, so we re-use a va_list. */
    va_list args;
    va_start(args, format);

    /* utils_snprintf does not accept va_list directly; forward by calling it
     * with the buffer and letting it consume the variadic pack internally.
     * Because utils_snprintf matches our format string exactly, we pass the
     * buffer and then manually call it with the same format + args expansion.
     *
     * Since utils_snprintf takes (...) rather than (va_list), we use a small
     * wrapper approach: build a local copy of the format call. */
    /* Note: utils_snprintf internally uses va_start/va_end, so we must call
     * it without the va_list wrapper. We close our va_list and call directly. */
    va_end(args);

    /* Direct call - works because utils_snprintf accepts variadic args */
    /* We replicate the call below using a macro-friendly expansion trick:
     * because C does not let us forward va_list to a (...) function easily,
     * we call serial_printf recursively would be wrong.  Instead we rely on
     * the fact that utils_snprintf accepts (...) and we forward with __VA_ARGS__
     * which is not available here.  The safe solution for a bare-metal project
     * without vsnprintf is to duplicate the va_list processing. */

    /* ---- Portable workaround ----
     * Call utils_snprintf through a local helper that accepts va_list by
     * re-opening the argument list and passing arguments one by one.
     * Since utils_snprintf is variadic we must use a trampoline.
     * For this course implementation we call it directly - the linker will
     * see the same argument stack frame.
     */

    /* Re-open args for the actual formatting call */
    va_start(args, format);
    /* Build formatted string - use the buffer */
    /* We need a vsnprintf equivalent. utils_snprintf is (char*, const char*, ...)
     * We simulate va_list forwarding by using a local vutils_snprintf that
     * accepts va_list. Since that doesn't exist in the provided utils module,
     * we implement the transmission loop manually matching the format string. */

    /* Simplest correct approach for the given utils API:
     * utils_snprintf writes to serial_buffer. We open a fresh va_list and
     * "re-call" it. In C99 the trick is to use a function pointer cast, but
     * the cleanest embedded solution is to duplicate utils_snprintf inline.
     * For this lab we call utils_snprintf with an explicit va_list by using
     * the __builtin_apply trick - but that is GCC-specific and fragile.
     *
     * FINAL DECISION: implement a local vformat function here that mirrors
     * utils_snprintf but accepts va_list, so we can forward cleanly.
     */
    va_end(args);

    /* --- Clean implementation without forwarding limitation --- */
    /* We cannot truly forward a va_list to a (...) function portably in C99.
     * The pragmatic solution for this embedded lab is to re-implement the
     * minimal formatting inline here using va_list directly, which is exactly
     * what the real vsnprintf does. This keeps the code self-contained. */

    char *dst = serial_buffer;
    const char *fmt = format;

    va_start(args, format);

    while (*fmt && (dst < serial_buffer + SERIAL_BUFFER_SIZE - 1))
    {
        if (*fmt == '%')
        {
            fmt++;
            switch (*fmt)
            {
                case 'd':
                {
                    int32_t val = va_arg(args, int32_t);
                    uint8_t tmp[16];
                    uint32_t len = utils_itoa(val, tmp, 1U, 10U);
                    for (uint32_t i = 0U; i < len && dst < serial_buffer + SERIAL_BUFFER_SIZE - 1; i++)
                    {
                        *dst++ = (char)tmp[i];
                    }
                    break;
                }
                case 'u':
                {
                    int32_t val = va_arg(args, int32_t);
                    uint8_t tmp[16];
                    uint32_t len = utils_itoa(val, tmp, 0U, 10U);
                    for (uint32_t i = 0U; i < len && dst < serial_buffer + SERIAL_BUFFER_SIZE - 1; i++)
                    {
                        *dst++ = (char)tmp[i];
                    }
                    break;
                }
                case 'x':
                {
                    int32_t val = va_arg(args, int32_t);
                    uint8_t tmp[16];
                    uint32_t len = utils_itoa(val, tmp, 0U, 16U);
                    for (uint32_t i = 0U; i < len && dst < serial_buffer + SERIAL_BUFFER_SIZE - 1; i++)
                    {
                        *dst++ = (char)tmp[i];
                    }
                    break;
                }
                case 's':
                {
                    char *s = va_arg(args, char *);
                    while (s && *s && dst < serial_buffer + SERIAL_BUFFER_SIZE - 1)
                    {
                        *dst++ = *s++;
                    }
                    break;
                }
                case 'c':
                {
                    *dst++ = (char)va_arg(args, int);
                    break;
                }
                case '%':
                {
                    *dst++ = '%';
                    break;
                }
                default:
                {
                    *dst++ = '%';
                    if (dst < serial_buffer + SERIAL_BUFFER_SIZE - 1)
                    {
                        *dst++ = *fmt;
                    }
                    break;
                }
            }
        }
        else
        {
            *dst++ = *fmt;
        }
        fmt++;
    }

    *dst = '\0';
    va_end(args);

    /* Transmit each character of the formatted buffer over UART */
    for (char *p = serial_buffer; *p != '\0'; p++)
    {
        uart_write((uint8_t)*p);
    }

    return SERIAL_OK;
}

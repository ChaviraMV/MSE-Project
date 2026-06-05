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
 * @file utils.c
 * @brief Utility library with helper functions.
 *
 * Utils module has helper functions to treat strings, ASCII conversions, and
 * printing utilities.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date 04/30/2026
 *
 */

/*** Includes ***/
#include "utils.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/

static uint32_t utils_printString(char *dst, char *src);
static uint32_t utils_printInt(char *dst, int32_t num, uint8_t sign, uint32_t base);

/*** Function Definitions ***/

void utils_snprintf(char *dst, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%')
        {
            format++;
            switch (*format)
            {
                /* Hint: on the data type cases use va_arg(args, data_type) */
                case 's':
                {
                    char *str = va_arg(args, char *);
                    uint32_t len = utils_printString(dst, str);
                    dst += len;
                    break;  
                } 
                case 'd':
                {
                    int32_t num = va_arg(args, int32_t);
                    uint32_t len = utils_printInt(dst, num, 1, 10);
                    dst += len;
                    break;
                }
                case 'u':
                {
                    uint32_t num = va_arg(args, uint32_t);
                    uint32_t len = utils_printInt(dst, (int32_t)num, 0, 10);
                    dst += len;
                    break;
                }
                case 'x':
                {
                    uint32_t num = va_arg(args, uint32_t);
                    uint32_t len = utils_printInt(dst, (int32_t)num, 0, 16);
                    dst += len;
                    break;  
                }
                case 'c':
                    char c = (char)va_arg(args, int);
                    *dst++ = c;
                    break;
                case '%':
                    *dst++ = '%';
                    break;
                default:
                    *dst++ = '%';
                    *dst++ = *format;
                    break;
            }
        }
        else
        {
            *dst++ = *format;
        }

        format++;
    }
    *dst = '\0';
    va_end(args);
}


/**
 * @brief Copy a string into a destination buffer.
 *
 * This function calculates the length of the source string and copies
 * its contents into the destination buffer.
 *
 * @param dst Pointer to the destination buffer where the string will be copied.
 * @param src Pointer to the source string to copy.
 *
 * @return Length of the string copied (number of characters).
 */
static uint32_t utils_printString(char *dst, char *src)
{
    uint32_t len = 0;

    while (*src)
    {
        *dst++ =*src++;
        len++;
    }
    return len;
}

/**
 * @brief Convert an integer to ASCII and copy it into a destination buffer.
 *
 * This function converts an integer into its ASCII representation
 * based on the specified base (2-16) and sign option.
 * The resulting string is copied into the destination buffer.
 *
 * @param dst Pointer to the destination buffer where the ASCII string will be stored.
 * @param num Integer number to convert.
 * @param sign Interger value that indicates if data is signed or unsigned.
 * @param base Numerical base for conversion.
 *
 * @return Length of the ASCII string copied into the destination buffer.
 */
static uint32_t utils_printInt(char *dst, int32_t num, uint8_t sign, uint32_t base)
{
    uint8_t tmp[32];
    uint32_t len = 0;

    len = utils_itoa(num, tmp, sign, base);
    utils_printString(dst, (char *) tmp);

    return len; 
}
/**
 * @brief Convert data from integer type into an ASCII string
 *
 * Given an integer value, this will convert a provided integer to
 * an ASCII string data type regardless of the integer base (2-16),
 * and return the number of digits of the converted ASCII string.
 * The numerical system of the integer is determined by the provided
 * base value.
 *
 * @param data Integer value to convert to
 * @param ptr Pointer to the ASCII string
 * @param sign Interger value that indicates if data is signed or unsigned
 * @param base Base of the integer to convert to
 *
 * @return Length of the converted data.
 */
uint32_t utils_itoa(int32_t data, uint8_t *ptr, uint8_t sign, uint8_t base)
{
    uint32_t len = 0;
    uint8_t tmp[32];
    uint32_t i = 0;
    uint32_t udata;

    if (sign && (data < 0))
    {
        *ptr++ = '-';
        udata = (uint32_t)(-data);
        len++;
    }
    else 
    {
        udata = (uint32_t)data;
    }
    do
    {
        uint8_t rem = udata % base;
        tmp[i++] = (rem < 10) ? ('0' + rem) : ('A' + rem - 10);
        udata /= base;
    } while (udata > 0);
    while (i > 0)
    {
        *ptr++ = tmp[--i];
        len++;
    }

    *ptr = '\0';

    return len;
}
/**
 * @brief Convert data from an ASCII string into an integer type
 *
 * Given an unsigned integer pointer, this will convert a pointer to
 * a character string to an integer data type regardless of the integer
 * base (2-16), and return the converted integer data.
 * The numerical system of the integer is determined by the provided
 * base value and the number of characters is determined by the provided
 * digits value.
 *
 * @param ptr Pointer to the ASCII string
 * @param digits Number of digits in the ASCII string
 * @param sign Interger value that indicates if data is signed or unsigned
 * @param base Base of the converted integer
 *
 * @return Converted integer value.
 */
int32_t utils_atoi(uint8_t *ptr, uint32_t digits, uint8_t sign, uint8_t base)
{
    int32_t result = 0;
    uint8_t negative = 0;

    if (sign && (*ptr == '-'))
    {
        negative = 1;
        ptr++;
        digits--;
    }
    while (digits > 0)
    {
        uint8_t digit;
         if (*ptr >= '0' && *ptr <= '9')
        {
            digit = *ptr - '0';
        }
        else if (*ptr >= 'A' && *ptr <= 'F')
        {
            digit = *ptr - 'A' + 10;
        }
        else if (*ptr >= 'a' && *ptr <= 'f')
        {
            digit = *ptr - 'a' + 10;
        }
        else
        {
            break;
        }

        result = result * base + digit;
        ptr++;
        digits--; 
    }
     if (negative)
    {
        result = -result;
    }

    return result;
}
/**
 * @brief Copy a block of memory from source to destination.
 *
 * This function copies a specified number of bytes from the source buffer
 * to the destination buffer. It assumes that the source and destination
 * buffers do not overlap.
 *
 * @param dst Pointer to the destination buffer where data will be copied.
 * @param src Pointer to the source buffer containing the data to copy.
 * @param length Number of bytes to copy from source to destination.
 *
 * @return Pointer to the destination buffer (dst).
 */
void * utils_memCpy(void *dst, void *src, size_t length)
{
    uint8_t *d = (uint8_t *)dst;
    uint8_t *s = (uint8_t *)src;
    
    while (length > 0)
    {
        *d++ = *s++;
        length--;
    }
    return dst;
}
/**
 * @brief Reverse the order of elements of a data set
 *
 * Given a pointer to an unsigned integer data set, this will reverse a number
 * of elements from a provided data set. The number of elements is determined
 * by the provided length parameter.
 *
 * @param src Pointer to source data set
 * @param length Number of elements to reverse on the data set
 *
 * @return Pointer to the source (src).
 */
void * utils_memReverse(void *src, size_t length)
{
    uint8_t *start = (uint8_t *)src;
    uint8_t *end = (uint8_t *)src + length - 1;
    uint8_t tmp;
    
    while (start < end)
    {
        tmp = *start;
        *start = *end;
        *end = tmp;

        start++;
        end--;
    }
    return src;
}
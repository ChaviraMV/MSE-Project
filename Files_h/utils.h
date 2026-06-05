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
 * @file utils.h
 * @brief Utility library with helper functions.
 *
 * Utils module has helper functions to treat strings, ASCII conversions, and
 * printing utilities.
 *
 *@author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date 04/29/2026
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__

/*** Includes ***/
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>  
/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Global Variables ***/

/*** Function Prototypes ***/

/**
 * @brief Format and store a string into a buffer
 *
 * This function builds a formatted string based on a format specifier string
 * and a variable list of arguments, similar to the standard snprintf.
 * The resulting string is written into the provided destination buffer.
 *
 * @param dst Pointer to the destination buffer where the formatted string will be stored.
 * @param format Format string containing text and conversion specifiers (e.g., %d, %s, %x).
 * @param ... Variable arguments corresponding to the format specifiers.
 *
 * @return None.
 */
void utils_snprintf(char *dst, const char *format, ...);

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
uint32_t utils_itoa(int32_t data, uint8_t *ptr, uint8_t sign, uint8_t base);

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
int32_t utils_atoi(uint8_t *ptr, uint32_t digits, uint8_t sign, uint8_t base);

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
void * utils_memCpy(void *dst, void *src, size_t length);

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
void * utils_memReverse(void *src, size_t length);

#endif /* __UTILS_H__ */

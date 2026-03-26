#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>

// bare metal va_list using gcc built ins so we dont need stdarg.h
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)

/**
 * @brief custom printf for lpuart1 serial port
 * * this function formats a string and prints it to the serial console.
 * @details supported format specifiers:
 * %c - character
 * %s - string
 * %d, %i - signed decimal integer
 * %u - unsigned decimal integer
 * %f - floating point number (double)
 * %x - unsigned hex integer (lowercase)
 * %X - unsigned hex integer (uppercase)
 * %b - unsigned binary integer
 * %o - unsigned octal integer
 * %p - pointer address
 * %% - literal '%' character
 * * supported flags:
 * '-' - left align output
 * '#' - alternative form (0x, 0b, 0)
 * '0' - zero-padding
 * * length modifiers: h, hh, l, ll supported.
 *
 * @param format the null-terminated format string
 * @param ... variable arguments
 * @return total number of characters printed
 */
int printf(const char *format, ...);

#endif
#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stdint.h>
#include "LPUART.h"

/**
 * @brief Standard sprintf implementation. Formats a string into a provided buffer.
 * @param buf The destination memory buffer.
 * @param format The format string (supports %d, %x, %s, %f, %c, %p, padding, etc.)
 * @return The number of characters written to the buffer.
 */
int sprintf(char *buf, const char *format, ...);

/**
 * @brief Variadic version of sprintf.
 */
int vsprintf(char *buf, const char *format, va_list args);

/**
 * @brief Prints a formatted string to the local debug console (LPUART1).
 * converts "\n" to "\r\n" for terminal compatibility.
 */
int print_dbg(const char *format, ...);

/**
 * @brief Sends a formatted raw string directly to the ESP8266 (LPUART4).
 */
int send_to_esp(const char *format, ...);

/**
 * @brief Wraps a formatted string in the AT+CIPSEND command and sends it to the ESP8266.
 */
int print_esp(const char *format, ...);

#endif // STDIO_H
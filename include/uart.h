#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "nxp_frdm_imx91.h"

/**
 * @brief Sends a single character over the specified LPUART.
 * @param uart Pointer to the LPUART instance (e.g., LPUART1).
 * @param c The character to send.
 */
void uart_putchar(LPUART_TypeDef *uart, char c);

/**
 * @brief Sends a null-terminated string over the specified LPUART.
 * @param uart Pointer to the LPUART instance.
 * @param str Pointer to the string.
 */
void uart_print_string(LPUART_TypeDef *uart, const char *str);

/**
 * @brief Prints a 32-bit integer in hexadecimal format.
 * @param uart Pointer to the LPUART instance.
 * @param val The 32-bit value to print.
 */
void uart_print_hex(LPUART_TypeDef *uart, uint32_t val);

/**
 * @brief Waits infinitely until a character is received. (Blocking)
 * @param uart Pointer to the LPUART instance.
 * @return The received character.
 */
char uart_getchar_blocking(LPUART_TypeDef *uart);

/**
 * @brief Checks for a received character and returns immediately. (Non-blocking)
 * @param uart Pointer to the LPUART instance.
 * @return The received character, or '\0' if no character is available.
 */
char uart_getchar_nonblocking(LPUART_TypeDef *uart);

#endif /* UART_H */
#include "LPUART.h"

void uart_putchar(LPUART_TypeDef *uart, char c) {
    /* Automatically inject a carriage return before a newline */
    if (c == '\n') {
        while (!(uart->STAT & LPUART_STAT_TDRE)); 
        uart->DATA = '\r';
    }
    
    /* Send the actual character */
    while (!(uart->STAT & LPUART_STAT_TDRE)); 
    uart->DATA = c;
}

void uart_print_string(LPUART_TypeDef *uart, const char *str) {
    while (*str) {
        uart_putchar(uart, *str++);
    }
}

void uart_print_hex(LPUART_TypeDef *uart, uint32_t val) {
    uart_print_string(uart, "0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        if (nibble < 10) uart_putchar(uart, '0' + nibble);
        else uart_putchar(uart, 'A' + (nibble - 10));
    }
}

char uart_getchar_blocking(LPUART_TypeDef *uart) {
    while (!(uart->STAT & LPUART_STAT_RDRF)); 
    return (char)(uart->DATA & 0xFF);
}

char uart_getchar_nonblocking(LPUART_TypeDef *uart) {
    if (uart->STAT & LPUART_STAT_RDRF) {
        return (char)(uart->DATA & 0xFF);
    }
    return '\0'; 
}

void uart_init(LPUART_TypeDef *uart, uint32_t baudrate, uint32_t src_clock_hz) {
    uint32_t sbr;

    /* Disable TX and RX before changing baud rate */
    uart->CTRL &= ~(LPUART_CTRL_TE | LPUART_CTRL_RE);

    /* Calculate Baud Rate Modulo Divisor (SBR)
     * Formula: SBR = Clock_Frequency / (16 * Baud_Rate) */
    sbr = src_clock_hz / (baudrate * 16);
    
    /* Mask out old SBR bits (bits 0-12) and write new ones */
    uart->BAUD &= ~0x1FFF; 
    uart->BAUD |= (sbr & 0x1FFF);

    /* Enable Transmitter and Receiver */
    uart->CTRL |= (LPUART_CTRL_TE | LPUART_CTRL_RE);
}

void initUART3(uint32_t baudrate, uint32_t src_clock_hz) {
    uart_init(LPUART3, baudrate, src_clock_hz);
}

void initUART4(uint32_t baudrate, uint32_t src_clock_hz) {
    uart_init(LPUART4, baudrate, src_clock_hz);
}
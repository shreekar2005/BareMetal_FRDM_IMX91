#include "LPUART.h"

void lpuart_putchar(LPUART_TypeDef *lpuart, char c) {
    /* Automatically inject a carriage return before a newline */
    if (c == '\n') {
        while (!(lpuart->STAT & LPUART_STAT_TDRE)); 
        lpuart->DATA = '\r';
    }
    
    /* Send the actual character */
    while (!(lpuart->STAT & LPUART_STAT_TDRE)); 
    lpuart->DATA = c;
}

void lpuart_print_string(LPUART_TypeDef *lpuart, const char *str) {
    while (*str) {
        lpuart_putchar(lpuart, *str++);
    }
}

void lpuart_print_hex(LPUART_TypeDef *lpuart, uint32_t val) {
    lpuart_print_string(lpuart, "0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        if (nibble < 10) lpuart_putchar(lpuart, '0' + nibble);
        else lpuart_putchar(lpuart, 'A' + (nibble - 10));
    }
}

char lpuart_getchar_blocking(LPUART_TypeDef *lpuart) {
    while (!(lpuart->STAT & LPUART_STAT_RDRF)); 
    return (char)(lpuart->DATA & 0xFF);
}

char lpuart_getchar_nonblocking(LPUART_TypeDef *lpuart) {
    if (lpuart->STAT & LPUART_STAT_RDRF) {
        return (char)(lpuart->DATA & 0xFF);
    }
    return '\0'; 
}

void lpuart_init(LPUART_TypeDef *lpuart, uint32_t baudrate, uint32_t src_clock_hz) {
    uint32_t sbr;

    /* Disable TX and RX before changing baud rate */
    lpuart->CTRL &= ~(LPUART_CTRL_TE | LPUART_CTRL_RE);

    /* Calculate Baud Rate Modulo Divisor (SBR)
     * Formula: SBR = Clock_Frequency / (16 * Baud_Rate) */
    sbr = src_clock_hz / (baudrate * 16);
    
    /* Mask out old SBR bits (bits 0-12) and write new ones */
    lpuart->BAUD &= ~0x1FFF; 
    lpuart->BAUD |= (sbr & 0x1FFF);

    /* Enable Transmitter and Receiver */
    lpuart->CTRL |= (LPUART_CTRL_TE | LPUART_CTRL_RE);
}

void initLPUART3(uint32_t baudrate, uint32_t src_clock_hz) {
    lpuart_init(LPUART3, baudrate, src_clock_hz);
}

void initLPUART4(uint32_t baudrate, uint32_t src_clock_hz) {
    lpuart_init(LPUART4, baudrate, src_clock_hz);
}
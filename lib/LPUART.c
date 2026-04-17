#include "LPUART.h"

void lpuartPutChar(LPUART_TypeDef *lpuart, char c) {    
    /* Send the actual character */
    while (!(lpuart->STAT & LPUART_STAT_TDRE)); 
    lpuart->DATA = c;
}

void lpuartPrintString(LPUART_TypeDef *lpuart, const char *str) {
    while (*str) {
        lpuartPutChar(lpuart, *str++);
    }
}

void lpuartPrintHex(LPUART_TypeDef *lpuart, uint32_t val) {
    lpuartPrintString(lpuart, "0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        if (nibble < 10) lpuartPutChar(lpuart, '0' + nibble);
        else lpuartPutChar(lpuart, 'A' + (nibble - 10));
    }
}

char lpuartGetCharBlocking(LPUART_TypeDef *lpuart) {
    while (!(lpuart->STAT & LPUART_STAT_RDRF)); 
    return (char)(lpuart->DATA & 0xFF);
}

char lpuartGetCharNonBlocking(LPUART_TypeDef *lpuart) {
    if (lpuart->STAT & LPUART_STAT_RDRF) {
        return (char)(lpuart->DATA & 0xFF);
    }
    return '\0'; 
}

void lpuartINIT(LPUART_TypeDef *lpuart, uint32_t baudrate, uint32_t src_clock_hz) {
    uint32_t sbr;

    /* Disable TX and RX before changing baud rate */
    lpuart->CTRL &= ~(LPUART_CTRL_TE | LPUART_CTRL_RE);

    /* Calculate Baud Rate Modulo Divisor (SBR)
     * Formula: SBR = Clock_Frequency / (16 * Baud_Rate) */
    sbr = src_clock_hz / (baudrate * 16);

    /* Clear both SBR (bits 0-12) AND OSR (bits 24-28) */
    lpuart->BAUD &= ~((0x1FFF) | (0x1F << 24)); 
    
    /* Set new SBR, and explicitly force OSR to 15 (which means 16x oversampling) */
    lpuart->BAUD |= (sbr & 0x1FFF) | (15 << 24);

    /* Flush the FIFOs to clear any garbage data */
    lpuart->FIFO |= (LPUART_FIFO_TXFLUSH | LPUART_FIFO_RXFLUSH);
    
    /* Enable both Transmit and Receive FIFOs */
    lpuart->FIFO |= (LPUART_FIFO_TXFE | LPUART_FIFO_RXFE);

    /* Enable Transmitter and Receiver */
    lpuart->CTRL |= (LPUART_CTRL_TE | LPUART_CTRL_RE);
}
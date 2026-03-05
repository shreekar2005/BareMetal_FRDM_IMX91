#ifndef LPUART_H
#define LPUART_H

#include <stdint.h>

/**
 * @brief LPUART Register Structure based on i.MX91 Reference Manual
 */
typedef struct {
    volatile uint32_t VERID;       /* 0x00 Version ID */
    volatile uint32_t PARAM;       /* 0x04 Parameter */
    volatile uint32_t GLOBAL;      /* 0x08 Global */
    volatile uint32_t PINCFG;      /* 0x0C Pin Configuration */
    volatile uint32_t BAUD;        /* 0x10 Baud Rate */
    volatile uint32_t STAT;        /* 0x14 Status */
    volatile uint32_t CTRL;        /* 0x18 Control */
    volatile uint32_t DATA;        /* 0x1C Data */
    volatile uint32_t MATCH;       /* 0x20 Match Address */
    volatile uint32_t MODIR;       /* 0x24 Modem IrDA */
    volatile uint32_t FIFO;        /* 0x28 FIFO */
    volatile uint32_t WATER;       /* 0x2C Watermark */
    volatile uint32_t DATARO;      /* 0x30 Data Read-Only */
    uint32_t RESERVED_0[3];        /* 0x34 - 0x3C Padding */
    volatile uint32_t MCR;         /* 0x40 Modem Control */
    volatile uint32_t MSR;         /* 0x44 Modem Status */
    volatile uint32_t REIR;        /* 0x48 Receiver Extended Idle */
    volatile uint32_t TEIR;        /* 0x4C Transmitter Extended Idle */
    volatile uint32_t HDCR;        /* 0x50 Half Duplex Control */
    uint32_t RESERVED_1;           /* 0x54 Padding */
    volatile uint32_t TOCR;        /* 0x58 Timeout Control */
    volatile uint32_t TOSR;        /* 0x5C Timeout Status */
    volatile uint32_t TIMEOUT[4];  /* 0x60 - 0x6C Timeout N (TIMEOUT0 - TIMEOUT3) */
    uint32_t RESERVED_2[100];      /* 0x70 - 0x1FC Padding */
    volatile uint32_t TCBR[128];   /* 0x200 - 0x3FC Transmit Command Burst (TCBR0 - TCBR127) */
    volatile uint32_t TDBR[256];   /* 0x400 - 0x7FC Transmit Data Burst (TDBR0 - TDBR255) */
} LPUART_TypeDef;

/* ==========================================
 * BASE ADDRESSES & PERIPHERAL POINTERS
 * ========================================== */

#define LPUART1_BASE 0x44380000
#define LPUART2_BASE 0x44390000
#define LPUART3_BASE 0x42570000
#define LPUART4_BASE 0x42580000
#define LPUART5_BASE 0x42590000
#define LPUART6_BASE 0x425A0000
#define LPUART7_BASE 0x42690000
#define LPUART8_BASE 0x426A0000

#define LPUART1 ((LPUART_TypeDef *) LPUART1_BASE)
#define LPUART2 ((LPUART_TypeDef *) LPUART2_BASE)
#define LPUART3 ((LPUART_TypeDef *) LPUART3_BASE)
#define LPUART4 ((LPUART_TypeDef *) LPUART4_BASE)
#define LPUART5 ((LPUART_TypeDef *) LPUART5_BASE)
#define LPUART6 ((LPUART_TypeDef *) LPUART6_BASE)
#define LPUART7 ((LPUART_TypeDef *) LPUART7_BASE)
#define LPUART8 ((LPUART_TypeDef *) LPUART8_BASE)

/* Bit Definitions */
#define LPUART_STAT_TDRE (1 << 23)
#define LPUART_STAT_RDRF (1 << 21)

/* ==========================================
 * UART FUNCTION PROTOTYPES
 * ========================================== */

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

#endif /* LPUART_H */
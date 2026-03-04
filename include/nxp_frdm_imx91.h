#ifndef NXP_FRDM_IMX91_H
#define NXP_FRDM_IMX91_H

#include <stdint.h>

/**
 * @brief LPUART Register Structure
 */
typedef struct {
    volatile uint32_t VERID;       /* 0x00 */
    volatile uint32_t PARAM;       /* 0x04 */
    volatile uint32_t GLOBAL;      /* 0x08 */
    volatile uint32_t PINCFG;      /* 0x0C */
    volatile uint32_t BAUD;        /* 0x10 */
    volatile uint32_t STAT;        /* 0x14 */
    volatile uint32_t CTRL;        /* 0x18 */
    volatile uint32_t DATA;        /* 0x1C */
} LPUART_TypeDef;

/**
 * @brief GPIO Register Structure
 */
typedef struct {
    volatile uint32_t VERID;       /* 0x00 */
    volatile uint32_t PARAM;       /* 0x04 */
    volatile uint32_t LOCK;        /* 0x08 */
    uint32_t RESERVED_0;           /* 0x0C Padding */
    volatile uint32_t PCNS;        /* 0x10 */
    volatile uint32_t ICNS;        /* 0x14 */
    volatile uint32_t PCNP;        /* 0x18 */
    volatile uint32_t ICNP;        /* 0x1C */
    uint32_t RESERVED_1[8];        /* 0x20 - 0x3C Padding */
    volatile uint32_t PDOR;        /* 0x40 Port Data Output */
    volatile uint32_t PSOR;        /* 0x44 Port Set Output */
    volatile uint32_t PCOR;        /* 0x48 Port Clear Output */
    volatile uint32_t PTOR;        /* 0x4C Port Toggle Output */
    volatile uint32_t PDIR;        /* 0x50 Port Data Input */
    volatile uint32_t PDDR;        /* 0x54 Port Data Direction */
} GPIO_TypeDef;


/* ==========================================
 * BASE ADDRESSES & PERIPHERAL POINTERS
 * ========================================== */

#define LPUART1_BASE 0x44380000
#define LPUART2_BASE 0x44390000
#define LPUART1 ((LPUART_TypeDef *) LPUART1_BASE)
#define LPUART2 ((LPUART_TypeDef *) LPUART2_BASE)

#define GPIO1_BASE 0x47400000
#define GPIO2_BASE 0x43810000
#define GPIO3_BASE 0x43820000
#define GPIO4_BASE 0x43830000
#define GPIO1 ((GPIO_TypeDef *) GPIO1_BASE)
#define GPIO2 ((GPIO_TypeDef *) GPIO2_BASE)
#define GPIO3 ((GPIO_TypeDef *) GPIO3_BASE)
#define GPIO4 ((GPIO_TypeDef *) GPIO4_BASE)

/* Bit Definitions */
#define LPUART_STAT_TDRE (1 << 23)
#define LPUART_STAT_RDRF (1 << 21)

#endif /* NXP_FRDM_IMX91_H */
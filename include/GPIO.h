#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/**
 * @brief GPIO Register Structure based on i.MX91 Reference Manual
 */
typedef struct {
    volatile uint32_t VERID;       /* 0x00 Version ID */
    volatile uint32_t PARAM;       /* 0x04 Parameter */
    uint32_t RESERVED_0;           /* 0x08 Padding */
    volatile uint32_t LOCK;        /* 0x0C Lock */
    volatile uint32_t PCNS;        /* 0x10 Pin Control Nonsecure */
    volatile uint32_t ICNS;        /* 0x14 Interrupt Control Nonsecure */
    volatile uint32_t PCNP;        /* 0x18 Pin Control Nonprivilege */
    volatile uint32_t ICNP;        /* 0x1C Interrupt Control Nonprivilege */
    uint32_t RESERVED_1[8];        /* 0x20 - 0x3C Padding */
    volatile uint32_t PDOR;        /* 0x40 Port Data Output */
    volatile uint32_t PSOR;        /* 0x44 Port Set Output */
    volatile uint32_t PCOR;        /* 0x48 Port Clear Output */
    volatile uint32_t PTOR;        /* 0x4C Port Toggle Output */
    volatile uint32_t PDIR;        /* 0x50 Port Data Input */
    volatile uint32_t PDDR;        /* 0x54 Port Data Direction */
    volatile uint32_t PIDR;        /* 0x58 Port Input Disable */
    uint32_t RESERVED_2;           /* 0x5C Padding */
    volatile uint8_t  PnDR[32];    /* 0x60 - 0x7F Pin Data (P0DR - P31DR) */
    volatile uint32_t ICR[32];     /* 0x80 - 0xFC Interrupt Control (ICR0 - ICR31) */
    volatile uint32_t GICLR;       /* 0x100 Global Interrupt Control Low */
    volatile uint32_t GICHR;       /* 0x104 Global Interrupt Control High */
    uint32_t RESERVED_3[6];        /* 0x108 - 0x11C Padding */
    volatile uint32_t ISFR[2];     /* 0x120 - 0x124 Interrupt Status Flag (ISFR0 - ISFR1) */
} GPIO_TypeDef;

/* ==========================================
 * BASE ADDRESSES & PERIPHERAL POINTERS
 * ========================================== */

#define GPIO1_BASE 0x47400000
#define GPIO2_BASE 0x43810000
#define GPIO3_BASE 0x43820000
#define GPIO4_BASE 0x43830000

#define GPIO1 ((GPIO_TypeDef *) GPIO1_BASE)
#define GPIO2 ((GPIO_TypeDef *) GPIO2_BASE)
#define GPIO3 ((GPIO_TypeDef *) GPIO3_BASE)
#define GPIO4 ((GPIO_TypeDef *) GPIO4_BASE)

#endif /* GPIO_H */
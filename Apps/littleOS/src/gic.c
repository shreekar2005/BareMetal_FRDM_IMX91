#include "include/gic.h"

#define GICD_BASE 0x48000000ULL
#define GICR_BASE 0x48040000ULL 
#define GICR_SGI_BASE 0x48050000ULL

#define GICD_CTLR        (*(volatile uint32_t*)(GICD_BASE + 0x0000))
#define GICR_WAKER       (*(volatile uint32_t*)(GICR_BASE + 0x0014))

void gic_init(void) {
    GICD_CTLR |= 2; 
    GICR_WAKER &= ~(1 << 1); 
    while(GICR_WAKER & (1 << 2)); 

    __asm__ volatile("msr S3_4_C12_C9_5, %0" : : "r" (9)); 
    __asm__ volatile("msr S3_0_C4_C6_0, %0" : : "r" (0xFF));
    __asm__ volatile("msr S3_0_C12_C12_7, %0" : : "r" (1));
}

void gic_enable_interrupt(uint32_t intid) {
    if (intid < 32) { 
        volatile uint32_t* igroupr0 = (volatile uint32_t*)(GICR_SGI_BASE + 0x0080);
        volatile uint32_t* isenabler0 = (volatile uint32_t*)(GICR_SGI_BASE + 0x0100);
        *igroupr0 |= (1 << intid);
        *isenabler0 = (1 << intid);
    } else { 
        uint32_t reg = intid / 32;
        uint32_t bit = intid % 32;
        volatile uint32_t* igroupr = (volatile uint32_t*)(GICD_BASE + 0x0080 + (reg * 4));
        volatile uint32_t* isenabler = (volatile uint32_t*)(GICD_BASE + 0x0100 + (reg * 4));
        *igroupr |= (1 << bit);
        *isenabler = (1 << bit);
    }
}

uint32_t gic_acknowledge_interrupt(void) {
    uint32_t iar;
    __asm__ volatile("mrs %0, S3_0_C12_C12_0" : "=r" (iar));
    return iar;
}

void gic_end_of_interrupt(uint32_t iar) {
    __asm__ volatile("msr S3_0_C12_C12_1, %0" : : "r" (iar));
}
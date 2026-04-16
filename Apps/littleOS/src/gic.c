#include "include/gic.h"
#include "include/common_macros.h"

extern void* vector_table; // Defined in your vector.S

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


void cpu_exceptions_init(void) {
    // Tell CPU where the Exception Vector Table is
    __asm__ volatile("msr vbar_el2, %0" : : "r" (&vector_table));

    // Route physical IRQs to Exception Level 2 (Hypervisor/OS Level)
    uint64_t hcr;
    __asm__ volatile("mrs %0, hcr_el2" : "=r" (hcr));
    hcr |= (1 << 4) | (1 << 3); 
    __asm__ volatile("msr hcr_el2, %0" : : "r" (hcr));

    // Unmask the CPU Master IRQ Pin (DAIF clear)
    __asm__ volatile("msr daifclr, #2");
}
#include "gic.h"
#include "LPUART.h"

#define GICD_BASE 0x48000000
#define GICR_BASE 0x48040000 
#define GICR_SGI_BASE 0x48050000 

#define GICD_CTLR        (*(volatile uint32_t*)(GICD_BASE + 0x0000))
#define GICR_WAKER       (*(volatile uint32_t*)(GICR_BASE + 0x0014))
#define GICR_IGROUPR0    (*(volatile uint32_t*)(GICR_SGI_BASE + 0x0080))
#define GICR_ISENABLER0  (*(volatile uint32_t*)(GICR_SGI_BASE + 0x0100))

void gic_init(void) {
    uart_print_string(LPUART1, "  -> [GIC] Enabling Distributor (GICD_CTLR)...\r\n");
    GICD_CTLR |= 2; 

    uart_print_string(LPUART1, "  -> [GIC] Waking up Redistributor (GICR_WAKER)...\r\n");
    GICR_WAKER &= ~(1 << 1); 
    
    uart_print_string(LPUART1, "  -> [GIC] Waiting for core to wake...\r\n");
    while(GICR_WAKER & (1 << 2)); 

    uart_print_string(LPUART1, "  -> [GIC] Configuring PPI 30 routing...\r\n");
    GICR_IGROUPR0 |= (1 << 30);
    GICR_ISENABLER0 = (1 << 30);

    uart_print_string(LPUART1, "  -> [GIC] Enabling System Registers (ICC_SRE_EL2)...\r\n");
    __asm__ volatile("msr S3_4_C12_C9_5, %0" : : "r" (9)); 

    uart_print_string(LPUART1, "  -> [GIC] Unmasking priorities (ICC_PMR_EL1)...\r\n");
    __asm__ volatile("msr S3_0_C4_C6_0, %0" : : "r" (0xFF));

    uart_print_string(LPUART1, "  -> [GIC] Enabling Group 1 CPU interface...\r\n");
    __asm__ volatile("msr S3_0_C12_C12_7, %0" : : "r" (1));
    
    uart_print_string(LPUART1, "  -> [GIC] Initialized Successfully!\r\n");
}

uint32_t gic_acknowledge_interrupt(void) {
    uint32_t iar;
    __asm__ volatile("mrs %0, S3_0_C12_C12_0" : "=r" (iar));
    return iar;
}

void gic_end_of_interrupt(uint32_t iar) {
    __asm__ volatile("msr S3_0_C12_C12_1, %0" : : "r" (iar));
}
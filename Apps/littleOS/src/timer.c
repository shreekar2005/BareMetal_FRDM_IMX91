#include <stdint.h>
#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/gic.h"

#define SCHEDULER_TICK_MS 20

extern void vector_table(void);
extern void register_irq(uint32_t intid, CPUState* (*handler)(CPUState*));

CPUState* timer_handler(CPUState* current_state) {
    uint32_t freq = sysctrGetFreq();
    uint32_t ticksFor20ms = freq / (1000 / SCHEDULER_TICK_MS);
    
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticksFor20ms));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));

    return os_schedule(current_state);
}

/**
 * @brief Initializes the hardware timer to generate periodic interrupts for the RTOS scheduler.
 */
void os_timer_init(void) {
    // Register with the OS software dispatcher
    register_irq(30, timer_handler);
    
    // Enable in the ARM GIC
    gic_enable_interrupt(30);

    // Configure the specific Timer Hardware
    uint32_t freq = sysctrGetFreq();
    uint32_t ticksFor20ms = freq / (1000 / SCHEDULER_TICK_MS);
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticksFor20ms));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
}
#include <stdint.h>
#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/gic.h"

static int current_timer_period_ms = 1; // Default to 1ms

extern void vector_table(void);
extern void register_irq(uint32_t intid, CPUState* (*handler)(CPUState*));

CPUState* timer_handler(CPUState* current_state) {
    uint32_t freq = sysctrGetFreq();
    
    // Calculate ticks for the dynamic period (freq / 1000 gives ticks per 1ms)
    uint32_t ticksForNext = (freq / 1000) * current_timer_period_ms;
    
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticksForNext));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));

    return os_schedule(current_state);
}

/**
 * @brief Initializes the hardware timer to generate periodic interrupts to invoke RTOS scheduler.
 * @param period_ms The interrupt frequency in milliseconds (e.g., 1 for 1000Hz).
 */
void os_timer_init(int period_ms) {
    if (period_ms <= 0) period_ms = 1; // Safety fallback
    current_timer_period_ms = period_ms;

    // Register with the OS software dispatcher
    register_irq(30, timer_handler);
    
    // Enable in the ARM GIC
    gicEnableInterrupt(30);

    // Configure the specific Timer Hardware
    uint32_t freq = sysctrGetFreq();
    uint32_t ticksForNext = (freq / 1000) * current_timer_period_ms;
    
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticksForNext));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
}
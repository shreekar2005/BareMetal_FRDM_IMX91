#include <stdint.h>
#include "SYS_CTR.h"
#include "kmultitasking.h"
#include "gic.h"

// The Preemptive Time Quantum!
#define SCHEDULER_TICK_MS 10 

extern void vector_table(void);

CPUState* timer_interrupt_handler(CPUState* current_state) {
    uint32_t iar = gic_acknowledge_interrupt();

    // Ensure this is the ARM Generic Timer (INTID 30)
    if (iar == 30) {
        uint32_t freq = sys_ctr_get_freq();
        uint32_t ticks_for_10ms = freq / (1000 / SCHEDULER_TICK_MS);
        
        // Reload countdown
        __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticks_for_10ms));
        __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
    }

    // Tell the GIC we are done, allowing the next tick to fire!
    gic_end_of_interrupt(iar);

    return schedule_tick(current_state);
}

void os_timer_init(void) {
    gic_init();

    __asm__ volatile("msr vbar_el2, %0" : : "r" (vector_table));

    // ========================================================
    // THE FIX: Route Physical Interrupts to EL2!
    // Bit 4 (IMO) = Route IRQs, Bit 3 (FMO) = Route FIQs
    // ========================================================
    uint64_t hcr;
    __asm__ volatile("mrs %0, hcr_el2" : "=r" (hcr));
    hcr |= (1 << 4) | (1 << 3); 
    __asm__ volatile("msr hcr_el2, %0" : : "r" (hcr));
    // ========================================================

    uint32_t freq = sys_ctr_get_freq();
    uint32_t ticks_for_10ms = freq / (1000 / SCHEDULER_TICK_MS);
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticks_for_10ms));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
    
    // Unmask core IRQs
    __asm__ volatile("msr daifclr, #2");
}
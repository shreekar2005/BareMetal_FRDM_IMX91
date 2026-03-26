#include <stdint.h>
#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/gic.h"

#define SCHEDULER_TICK_MS 20

extern void vector_table(void);
extern void register_irq(uint32_t intid, CPUState* (*handler)(CPUState*));

CPUState* timer_handler(CPUState* current_state) {
    uint32_t freq = sys_ctr_get_freq();
    uint32_t ticks_for_20ms = freq / (1000 / SCHEDULER_TICK_MS);
    
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticks_for_20ms));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));

    return schedule_tick(current_state);
}

void os_timer_init(void) {
    register_irq(30, timer_handler);
    gic_enable_interrupt(30);

    __asm__ volatile("msr vbar_el2, %0" : : "r" (vector_table));

    uint64_t hcr;
    __asm__ volatile("mrs %0, hcr_el2" : "=r" (hcr));
    hcr |= (1 << 4) | (1 << 3); 
    __asm__ volatile("msr hcr_el2, %0" : : "r" (hcr));

    uint32_t freq = sys_ctr_get_freq();
    uint32_t ticks_for_20ms = freq / (1000 / SCHEDULER_TICK_MS);
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (ticks_for_20ms));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
    
    __asm__ volatile("msr daifclr, #2");
}
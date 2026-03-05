#include "SYS_CTR.h"

uint64_t sys_ctr_get_ticks(void) {
    uint64_t ticks;
    /* Read the 64-bit physical timer count register */
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (ticks));
    return ticks;
}

uint32_t sys_ctr_get_freq(void) {
    uint32_t freq;
    /* Read the 32-bit timer frequency register */
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (freq));
    return freq;
}

void sys_ctr_delay_us(uint32_t us) {
    uint64_t start_ticks = sys_ctr_get_ticks();
    
    /* Calculate how many CPU ticks equal the requested microseconds */
    uint64_t wait_ticks = ((uint64_t)us * sys_ctr_get_freq()) / 1000000;
    
    /* Loop until the CPU counter surpasses our target */
    while ((sys_ctr_get_ticks() - start_ticks) < wait_ticks) {
        /* NOP (No Operation) prevents aggressive compiler optimization */
        __asm__ volatile("nop");
    }
}

void sys_ctr_delay_ms(uint32_t ms) {
    sys_ctr_delay_us(ms * 1000);
}
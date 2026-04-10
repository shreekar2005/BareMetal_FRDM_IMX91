#include "SYS_CTR.h"

uint64_t sysctrGetTicks(void) {
    uint64_t ticks;
    /* Read the 64-bit physical timer count register */
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (ticks));
    return ticks;
}

uint32_t sysctrGetFreq(void) {
    uint32_t freq;
    /* Read the 32-bit timer frequency register */
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (freq));
    return freq;
}

void sysctrDelayus(uint32_t us) {
    uint64_t start_ticks = sysctrGetTicks();
    
    /* Calculate how many CPU ticks equal the requested microseconds */
    uint64_t wait_ticks = ((uint64_t)us * sysctrGetFreq()) / 1000000;
    
    /* Loop until the CPU counter surpasses our target */
    while ((sysctrGetTicks() - start_ticks) < wait_ticks) {
        /* NOP (No Operation) prevents aggressive compiler optimization */
        __asm__ volatile("nop");
    }
}

void sysctrDelayms(uint32_t ms) {
    sysctrDelayus(ms * 1000);
}
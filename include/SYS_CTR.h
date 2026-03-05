#ifndef SYS_CTR_H
#define SYS_CTR_H

#include <stdint.h>

/**
 * @brief Reads the 64-bit ARM Generic Timer physical count (CNTPCT_EL0).
 * @return The current tick count directly from the CPU core.
 */
uint64_t sys_ctr_get_ticks(void);

/**
 * @brief Reads the timer frequency in Hz (CNTFRQ_EL0).
 * @return The hardware frequency (usually 24,000,000 Hz on i.MX).
 */
uint32_t sys_ctr_get_freq(void);

/**
 * @brief Blocks execution for a precise number of microseconds.
 * @param us Microseconds to delay.
 */
void sys_ctr_delay_us(uint32_t us);

/**
 * @brief Blocks execution for a precise number of milliseconds.
 * @param ms Milliseconds to delay.
 */
void sys_ctr_delay_ms(uint32_t ms);

#endif /* SYS_CTR_H */
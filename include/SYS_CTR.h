#ifndef SYS_CTR_H
#define SYS_CTR_H

#include <stdint.h>

/**
 * @brief Reads the 64-bit ARM Generic Timer physical count (CNTPCT_EL0).
 * @return The current tick count directly from the CPU core. ideal for precise timing and delays.
 */
uint64_t sysctrGetTicks(void);

/**
 * @brief Reads the timer frequency in Hz (CNTFRQ_EL0).
 * @return The hardware frequency (usually 24,000,000 Hz on i.MX).
 */
uint32_t sysctrGetFreq(void);

/**
 * @brief Blocks execution for a precise number of microseconds.
 * @param us Microseconds to delay.
 */
void sysctrDelay_us(uint32_t us);

/**
 * @brief Blocks execution for a precise number of milliseconds.
 * @param ms Milliseconds to delay.
 */
void sysctrDelay_ms(uint32_t ms);

#endif /* SYS_CTR_H */
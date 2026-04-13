#ifndef TIMER_H
#define TIMER_H

#include "include/multitasking.h"

static int current_timer_period_ms;

CPUState* timer_handler(CPUState* current_state);

/**
 * @brief Initializes the hardware timer to generate periodic interrupts to invoke RTOS scheduler.
 * @param period_ms The interrupt frequency in milliseconds (e.g., 1 for 1000Hz).
 */
void os_timer_init(int period_ms);

#endif // TIMER_H
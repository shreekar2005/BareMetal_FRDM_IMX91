#ifndef _GIC_H
#define _GIC_H

#include <stdint.h>

/**
 * @brief Initializes the CPU Core Exception state.
 * Must be called exactly once during boot.
 */
void cpu_exceptions_init(void);

/**
 * @brief initializes generic interrupt controller
 */
void gic_init(void);

/**
 * @brief enables specific interrupt id
 * @param intid interrupt number to unmask
 */
void gic_enable_interrupt(uint32_t intid);

/**
 * @brief reads hardware register to get fired interrupt id
 * @return interrupt id
 */
uint32_t gic_acknowledge_interrupt(void);

/**
 * @brief tells hardware we finished handling interrupt
 * @param iar interrupt id we just handled
 */
void gic_end_of_interrupt(uint32_t iar);

#endif
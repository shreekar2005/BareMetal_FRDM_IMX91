#ifndef _GIC_H
#define _GIC_H

#include <stdint.h>

/**
 * @brief Initializes the CPU Core Exception state.
 * Must be called exactly once during boot.
 */
void cpuExceptionsInit(void);

/**
 * @brief initializes generic interrupt controller
 */
void gicINIT(void);

/**
 * @brief enables specific interrupt id
 * @param intid interrupt number to unmask
 */
void gicEnableInterrupt(uint32_t intid);

/**
 * @brief reads hardware register to get fired interrupt id
 * @return interrupt id
 */
uint32_t gicAcknowledgeInterrupt(void);

/**
 * @brief tells hardware we finished handling interrupt
 * @param iar interrupt id we just handled
 */
void gicEndOfInterrupt(uint32_t iar);

#endif
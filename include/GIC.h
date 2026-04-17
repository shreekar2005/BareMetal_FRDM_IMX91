#ifndef _GIC_H
#define _GIC_H

#include <stdint.h>

#define GICD_BASE       0x48000000ULL // Base address for GIC Distributor
#define GICR_BASE       0x48040000ULL // Base address for GIC Redistributor
#define GICR_SGI_BASE   0x48050000ULL // Base address for GIC Redistributor SGI registers
#define GICD_CTLR       (*(volatile uint32_t*)(GICD_BASE + 0x0000)) // GIC Distributor Control Register
#define GICR_WAKER      (*(volatile uint32_t*)(GICR_BASE + 0x0014)) // GIC Redistributor WAKER Register

/**
 * @brief Initializes the CPU Core Exception state.
 * Must be called to give vector table address to CPU.
 * @param vector_table_ptr the address of the exception vector table (defined in assembly in vector.S)
 */
void gicCPUInit(uintptr_t vector_table_ptr);

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
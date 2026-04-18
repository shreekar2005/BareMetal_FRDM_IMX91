#include <stdint.h>
#include "GIC.h"
#include "include/multitasking.h"
#include "include/stdio.h"

typedef CPUState* (*irq_handler_t)(CPUState*);


/**
 * @brief Registers an IRQ handler function for a specific interrupt ID. This allows the OS to know which function to execute when a particular hardware interrupt fires. Must be called during system initialization for each interrupt you want to handle.
 * @param intid The interrupt ID (e.g., 30 for the timer, 101 for LPUART4 RX, etc.)
 * @param handler The function pointer to the handler that should be executed when this interrupt fires. The handler must match the signature: CPUState* handler(CPUState* current_state);
 */
void irq_register(uint32_t intid, irq_handler_t handler);

/**
 * @brief Centralized IRQ Dispatcher called from the assembly vector.S when any physical IRQ fires. It reads the interrupt ID from the GIC, looks up the corresponding handler in the isr_table, executes it, and then signals end of interrupt to the GIC before returning the next CPU state to execute.
 * @param current_state The CPU state at the time of the interrupt (passed from assembly).
 * @return The CPU state to switch to after handling the interrupt (either the same or a new one if the handler triggered a context switch).
 */
CPUState* irq_dispatcher(CPUState* current_state);

/**
 * @brief Initializes the CPU Core Exception state.
 * Must be called exactly once during boot.
 */
void gicCPUExceptionsInit(void);
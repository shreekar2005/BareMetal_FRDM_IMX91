#include <stdint.h>
#include "kmultitasking.h"
#include "gic.h"
#include "LPUART.h"

// Define a function pointer type for interrupt handlers
typedef CPUState* (*irq_handler_t)(CPUState*);

// The routing table: 1024 slots for 1024 possible GIC interrupts
static irq_handler_t isr_table[1024] = {0};

// Hardware drivers call this to plug themselves into the system
void register_irq(uint32_t intid, irq_handler_t handler) {
    if (intid < 1024) {
        isr_table[intid] = handler;
    }
}

// The Master Doorbell Answerer (Called by vector.S)
CPUState* irq_dispatcher(CPUState* current_state) {
    uint32_t iar = gic_acknowledge_interrupt();
    CPUState* next_state = current_state;

    // 1023 is a special "spurious/fake" interrupt ID we ignore
    if (iar < 1020) { 
        // Did a driver register itself for this ID?
        if (isr_table[iar] != 0) {
            // Yes! Jump to the specific driver's code
            next_state = isr_table[iar](current_state);
        } else {
            // No driver found! Print a warning so we can debug.
            uart_print_string(LPUART1, "\r\n[Kernel] Warning: Unhandled IRQ fired!\r\n");
        }
        
        // Always tell the GIC we are done
        gic_end_of_interrupt(iar);
    }

    return next_state;
}
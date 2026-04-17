#include <stdint.h>
#include "include/multitasking.h"
#include "GIC.h"
#include "include/stdio.h"
#include "include/irq.h"

static irq_handler_t isr_table[1024] = {0};

void register_irq(uint32_t intid, irq_handler_t handler) {
    if (intid < 1024) {
        isr_table[intid] = handler;
    }
}

CPUState* irq_dispatcher(CPUState* current_state) {
    uint32_t iar = gicAcknowledgeInterrupt();
    CPUState* next_state = current_state;

    if (iar < 1020) { 
        if (isr_table[iar] != 0) {
            next_state = isr_table[iar](current_state);
        } else {
            print_dbg("\n[IRQ-Driver] Warning: Unhandled IRQ fired!\n");
        }
        
        gicEndOfInterrupt(iar);
    }

    return next_state;
}

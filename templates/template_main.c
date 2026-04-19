#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "GIC.h"
#include "GPIO.h"
#include "IOMUX.h"
#include "LPUART.h"
#include "SYS_CTR.h"

extern void* vector_table; // Defined in vector.S

/** @brief Main entry point for __APP_NAME__ */
int main(void);

/** @brief Initialize hardware components (keeping it universal to avoid conflicts) */
void hardware_init(void);


int main(void) {
    hardware_init(); // initializing hardware
    lpuartPrintString(LPUART1, "\r\nHello from __APP_NAME__ bare-metal!\r\n");
    return 0;
}

void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    lpuartINIT(LPUART1, 115200, 24000000); // U-Boot already initialized LPUART1.

    // Hardware initialization for Generic Interrupt Controller
    gicINIT(); // Initialize the GIC Distributor and Redistributor
    gicCPUInit((uintptr_t)&vector_table); // Giving CPU vector_table address
}

#include <stdint.h>
#include "GPIO.h"
#include "GIC.h"
#include "LPUART.h"
#include "SYS_CTR.h"

extern void* vector_table; // Defined in vector.S

void hardware_init(void);
int main(void);

void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    lpuartINIT(LPUART1, 115200, 24000000); // U-Boot already initialized LPUART1.

    // Hardware initialization for Generic Interrupt Controller
    gicINIT(); // Initialize the GIC Distributor and Redistributor
    gicCPUInit((uintptr_t)&vector_table); // Giving CPU vector_table address
}


/**
 * @brief Main entry point for demo
 */
int main() {
    /* Initialize your hardware here */
    hardware_init();
    lpuartPrintString(LPUART1, "Hello from demo bare-metal!\n");
    return 0;
}
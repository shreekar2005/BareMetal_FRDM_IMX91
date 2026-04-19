#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "GIC.h"
#include "GPIO.h"
#include "IOMUX.h"
#include "LPUART.h"
#include "SYS_CTR.h"

#define LED_PIN (1 << 4)

extern void* vector_table; // Defined in vector.S

/** @brief Main entry point for helloWorld */
int main(void);

/** @brief Initialize hardware components (keeping it universal to avoid conflicts) */
void hardware_init(void);


int main() {
    GPIO2->PDDR |= LED_PIN; // output mode
    lpuartPrintString(LPUART1, "Hello from hello_world bare-metal!\r\n");
    lpuartPrintString(LPUART1, " press 's' to start/stop blinking LED\r\n press 'q' to quit\r\n");

    char run=0;
    while(1) {
        char input = lpuartGetCharNonBlocking(LPUART1);
        if (input != '\0') {
            lpuartPrintString(LPUART1, "Keyboard Input Detected: ");
            lpuartPutChar(LPUART1, input);
            lpuartPrintString(LPUART1, "\r\n");
            if(input== 's') run=~run;
            if(input == 'q') return 0;
        }
        if(!run) continue;

        GPIO2->PSOR = LED_PIN; // on
        sysctrDelay_ms(100);        
        
        GPIO2->PCOR = LED_PIN; // off
        sysctrDelay_ms(100);        
    }
    
    return 0;
}

void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    lpuartINIT(LPUART1, 115200, 24000000); // U-Boot already initialized LPUART1.

    // Hardware initialization for Generic Interrupt Controller
    gicINIT(); // Initialize the GIC Distributor and Redistributor
    gicCPUInit((uintptr_t)&vector_table); // Giving CPU vector_table address
}
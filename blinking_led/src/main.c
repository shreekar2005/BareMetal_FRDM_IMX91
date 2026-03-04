#include <stdint.h>
#include "nxp_frdm_imx91.h"
#include "uart.h"

#define LED_PIN (1 << 4)

void delay(volatile uint32_t count) {
    while(count--) __asm__ volatile("nop");
}

int main() {
    /* Set Pin 4 as an Output using the new struct */
    GPIO2->PDDR |= LED_PIN;

    uart_print_string(LPUART1, "\n\n-----------------------------------\n");
    uart_print_string(LPUART1, " MODULAR BARE METAL SYSTEM ACTIVE \n");
    uart_print_string(LPUART1, "-----------------------------------\n");

    char run=0;
    while(1) {

        char input = uart_getchar_nonblocking(LPUART1);
        if (input != '\0') {
            uart_print_string(LPUART1, "\nKeyboard Input Detected: ");
            uart_putchar(LPUART1, input);
            uart_print_string(LPUART1, "\n");
            if(input='r') run=~run;
        }
        if(!run) continue;
        /* Turn ON */
        GPIO2->PSOR = LED_PIN;  
        delay(10000000);        
        
        /* Turn OFF */
        GPIO2->PCOR = LED_PIN;  
        delay(10000000);        
    }
    
    return 0;
}
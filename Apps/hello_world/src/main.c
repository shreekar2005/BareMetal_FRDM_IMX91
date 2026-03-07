#include <stdint.h>
#include "GPIO.h"
#include "LPUART.h"

#define LED_PIN (1 << 4)

void delay(volatile uint32_t count) {
    while(count--) __asm__ volatile("nop");
}

/**
 * @brief Main entry point for blink_led
 */
int main() {
    GPIO2->PDDR |= LED_PIN; /* Set Pin 4 as an Output using the new struct */
    uart_print_string(LPUART1, "Hello from hello_world bare-metal!\n");
    uart_print_string(LPUART1, " press 's' to start/stop blinking LED\n press 'q' to quit\n");

    char run=0;
    while(1) {
        char input = uart_getchar_nonblocking(LPUART1);
        if (input != '\0') {
            uart_print_string(LPUART1, "Keyboard Input Detected: ");
            uart_putchar(LPUART1, input);
            uart_putchar(LPUART1, '\n');
            if(input== 's') run=~run;
            if(input == 'q') return 0;
        }
        if(!run) continue;

        GPIO2->PSOR = LED_PIN; /* Turn ON */ 
        delay(10000000);        
        
        GPIO2->PCOR = LED_PIN; /* Turn OFF */
        delay(10000000);        
    }
    
    return 0;
}
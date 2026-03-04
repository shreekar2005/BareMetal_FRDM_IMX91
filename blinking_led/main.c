// main.c
#include <stdint.h>

/* Physical base address for what Linux calls 'gpiochip0' */
#define GPIO2_BASE 0x43810000

/* Register Offsets from i.MX91 Manual */
#define GPIO_PSOR_OFFSET 0x44  /* Port Set Output */
#define GPIO_PCOR_OFFSET 0x48  /* Port Clear Output */
#define GPIO_PDDR_OFFSET 0x54  /* Port Data Direction */

/* Volatile Register Pointers */
#define GPIO2_PSOR (*(volatile uint32_t *)(GPIO2_BASE + GPIO_PSOR_OFFSET))
#define GPIO2_PCOR (*(volatile uint32_t *)(GPIO2_BASE + GPIO_PCOR_OFFSET))
#define GPIO2_PDDR (*(volatile uint32_t *)(GPIO2_BASE + GPIO_PDDR_OFFSET))

/* Pin Mask for Pin 4 */
#define LED_PIN (1 << 4)

/* Crude delay loop */
void delay(volatile uint32_t count) {
    while(count--) {
        __asm__ volatile("nop");
    }
}

int main() {
    /* 1. Set Pin 4 as an Output 
       Write 1 to Port Data Direction Register (PDDR) bit 4 */
    GPIO2_PDDR |= LED_PIN;

    /* 2. Infinite Blink Loop */
    while(1) {
        /* Write 1 to Port Set Output (PSOR) to turn ON */
        GPIO2_PSOR = LED_PIN;  
        delay(10000000);        
        
        /* Write 1 to Port Clear Output (PCOR) to turn OFF */
        GPIO2_PCOR = LED_PIN;  
        delay(10000000);        
    }
    
    return 0;
}
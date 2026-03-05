#include <stdint.h>
#include "GPIO.h"
#include "LPUART.h"
#include "SYS_CTR.h"

/* --- Configuration --- */
#define TRIG_PIN  2  /* GPIO2_02 */
#define ECHO_PIN  3  /* GPIO2_03 */
#define RED_LED   13 /* GPIO2_13 */

#define ALARM_THRESHOLD_CM 15 

void uart_print_dec(LPUART_TypeDef *uart, uint32_t val) {
    char buffer[10];
    int i = 0;
    if (val == 0) {
        uart_putchar(uart, '0');
        return;
    }
    while (val > 0) {
        buffer[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) {
        uart_putchar(uart, buffer[--i]);
    }
}

uint32_t sonar_read(void) {
    /* 1. Send 10-microsecond HIGH pulse */
    GPIO2->PSOR = (1 << TRIG_PIN);
    sys_ctr_delay_us(10);
    GPIO2->PCOR = (1 << TRIG_PIN);

    /* 2. Wait for Echo pin to go HIGH */
    uint64_t timeout_start = sys_ctr_get_ticks();
    uint64_t timeout_limit = (sys_ctr_get_freq() / 1000) * 38; /* Increased to 38ms for max range */
    
    while (!(GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sys_ctr_get_ticks() - timeout_start) > timeout_limit) return 0; 
    }

    /* 3. Record start time */
    uint64_t start_time = sys_ctr_get_ticks();

    /* 4. Wait for Echo pin to go LOW */
    while ((GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sys_ctr_get_ticks() - start_time) > timeout_limit) return 0; 
    }

    /* 5. Calculate distance */
    uint64_t end_time = sys_ctr_get_ticks();
    uint64_t total_ticks = end_time - start_time;
    uint32_t duration_us = (total_ticks * 1000000ULL) / sys_ctr_get_freq();

    return duration_us / 58;
}

int main() {
    uart_print_string(LPUART1, "\n--- HC-SR04 Bare-Metal Initializing ---\n");
    uart_print_string(LPUART1, "Press Ctrl+C to exit.\n\n");

    /* Hardware Init */
    GPIO2->PDDR |= (1 << RED_LED);
    GPIO2->PSOR = (1 << RED_LED); /* LED OFF */

    GPIO2->PDDR |= (1 << TRIG_PIN);
    GPIO2->PCOR = (1 << TRIG_PIN);

    GPIO2->PDDR &= ~(1 << ECHO_PIN);

    while (1) {
        /* --- Ctrl+C Intercept --- */
        char c = uart_getchar_nonblocking(LPUART1);
        if (c == 0x03) { /* ASCII HEX code for ETX (Ctrl+C) */
            uart_print_string(LPUART1, "\n[!] Ctrl+C caught! Shutting down Sonar...\n");
            
            /* Clean up hardware before exiting */
            GPIO2->PSOR = (1 << RED_LED); /* Ensure LED is off */
            GPIO2->PCOR = (1 << TRIG_PIN); /* Ensure trigger is low */
            
            break; /* Break out of the infinite loop */
        }

        /* --- Sonar Logic --- */
        uint32_t distance = sonar_read();

        if (distance == 0) {
            uart_print_string(LPUART1, "Out of range or disconnected!\n");
            GPIO2->PSOR = (1 << RED_LED);
        } else {
            uart_print_string(LPUART1, "Distance: ");
            uart_print_dec(LPUART1, distance);
            uart_print_string(LPUART1, " cm\n");

            if (distance < ALARM_THRESHOLD_CM) {
                GPIO2->PCOR = (1 << RED_LED); /* LED ON */
            } else {
                GPIO2->PSOR = (1 << RED_LED); /* LED OFF */
            }
        }

        sys_ctr_delay_ms(100);
    }

    /* Return to whatever launched us (usually causes a soft hang in bare-metal until reset) */
    return 0; 
}
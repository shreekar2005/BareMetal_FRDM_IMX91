#include <stdint.h>
#include "GPIO.h"
#include "LPUART.h"
#include "SYS_CTR.h"

/* --- Configuration --- */
#define TRIG_PIN  2  /* GPIO2_02 */
#define ECHO_PIN  3  /* GPIO2_03 */
#define RED_LED   13 /* GPIO2_13 : THIS IS ACTIVE LOW PIN */ 

/* Using millimeters for 1-decimal precision */
#define ALARM_THRESHOLD_MM 100  /* 10.0 cm */
#define MAX_DISTANCE_MM    4000 /* 400.0 cm */

#define NUM_READINGS    5 /* number of readings to take before returning median of distances (keep odd number >=3) */

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

/**
 * @brief Measures distance and returns millimeters for higher precision
 */
uint32_t sonar_read_mm(void) {
    /* 1. Send mandatory 10-microsecond HIGH pulse */
    GPIO2->PSOR = (1 << TRIG_PIN);
    sys_ctr_delay_us(10);
    GPIO2->PCOR = (1 << TRIG_PIN);

    /* 2. Wait for Echo pin to go HIGH */
    uint64_t timeout_start = sys_ctr_get_ticks();
    
    /* 24ms timeout is perfectly calibrated for ~400cm max hardware range */
    uint64_t timeout_limit = (sys_ctr_get_freq() / 1000) * 24; 
    
    while (!(GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sys_ctr_get_ticks() - timeout_start) > timeout_limit) return 0xFFFFFFFF; 
    }

    /* 3. Record start time */
    uint64_t start_time = sys_ctr_get_ticks();

    /* 4. Wait for Echo pin to go LOW */
    while ((GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sys_ctr_get_ticks() - start_time) > timeout_limit) return 0xFFFFFFFF; 
    }

    /* 5. Calculate precise distance */
    uint64_t end_time = sys_ctr_get_ticks();
    uint64_t total_ticks = end_time - start_time;
    uint32_t duration_us = (total_ticks * 1000000ULL) / sys_ctr_get_freq();

    /* duration_us / 58 = cm. Multiplying by 10 first gives us millimeters! */
    return (duration_us * 10) / 58;
}

/**
 * @brief Takes {NUM_READINGS} rapid readings and returns the median to eliminate noise/outliers
 */
uint32_t sonar_read_filtered_mm(void) {
    uint32_t readings[NUM_READINGS];
    
    /* Take 3 quick samples */
    for(int i = 0; i < NUM_READINGS; i++) {
        readings[i] = sonar_read_mm();
        /* Wait 10ms between pings so the previous sound wave can die out */
        sys_ctr_delay_ms(10); 
    }
    
    /* Simple Bubble Sort to order the 3 readings from smallest to largest */
    for (int i = 0; i < NUM_READINGS-1; i++) {
        for (int j = 0; j < NUM_READINGS-1-i; j++) {
            if (readings[j] > readings[j + 1]) {
                uint32_t temp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = temp;
            }
        }
    }
    
    /* The outlier spikes are now at the ends of the array. Return the middle! */
    uint32_t minOfMiddles = (readings[NUM_READINGS/2 -1]+readings[NUM_READINGS/2]+readings[NUM_READINGS/2+1])/3;
    return minOfMiddles;
}

int main() {
    uart_print_string(LPUART1, "\n--- HC-SR04 High-Precision Radar ---\n");
    uart_print_string(LPUART1, "Press Ctrl+C to exit.\n\n");

    /* Hardware Init */
    GPIO2->PDDR |= (1 << RED_LED);
    /* ACTIVE-HIGH: PCOR drives 0V to turn LED OFF */
    GPIO2->PCOR = (1 << RED_LED); 

    GPIO2->PDDR |= (1 << TRIG_PIN);
    GPIO2->PCOR = (1 << TRIG_PIN);

    GPIO2->PDDR &= ~(1 << ECHO_PIN);

    uint8_t led_state = 0; /* Keeps track of the blink cycle */

    while (1) {
        /* --- Ctrl+C Intercept --- */
        char c = uart_getchar_nonblocking(LPUART1);
        if (c == 0x03) { 
            uart_print_string(LPUART1, "\n[!] Ctrl+C caught! Shutting down Sonar...\n");
            GPIO2->PCOR = (1 << RED_LED); /* Ensure LED is off */
            GPIO2->PCOR = (1 << TRIG_PIN); 
            break; 
        }

        /* --- Sonar Logic --- */
        uint32_t distance_mm = sonar_read_filtered_mm();

        /* If out of bounds or timed out */
        if (distance_mm == 0xFFFFFFFF || distance_mm > MAX_DISTANCE_MM) {
            uart_print_string(LPUART1, "Distance: > 400.0 cm\n");
            GPIO2->PCOR = (1 << RED_LED); /* ACTIVE-HIGH: LED OFF */
            led_state = 0;
        } else {
            /* Print decimal formatted output (e.g. 15.3 cm) */
            uart_print_string(LPUART1, "Distance: ");
            uart_print_dec(LPUART1, distance_mm / 10); /* The whole centimeters */
            uart_putchar(LPUART1, '.');
            uart_print_dec(LPUART1, distance_mm % 10); /* The millimeter remainder */
            uart_print_string(LPUART1, " cm\n");

            /* Proximity Blink Logic (< 10 cm) */
            if (distance_mm < ALARM_THRESHOLD_MM) {
                led_state = !led_state; /* Flip the bit */
                if (led_state) {
                    GPIO2->PSOR = (1 << RED_LED); /* ACTIVE-HIGH: LED ON */
                } else {
                    GPIO2->PCOR = (1 << RED_LED); /* ACTIVE-HIGH: LED OFF */
                }   
            } else {
                GPIO2->PCOR = (1 << RED_LED); /* ACTIVE-HIGH: Force LED OFF if safe */
                led_state = 0;
            }
        }
    }

    return 0; 
}
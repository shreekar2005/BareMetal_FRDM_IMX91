#include <stdint.h>
#include "GPIO.h"
#include "LPUART.h"
#include "SYS_CTR.h"

#define TRIG_PIN  2  // GPIO2_02
#define ECHO_PIN  3  // GPIO2_03
#define RED_LED   13 // GPIO2_13 

#define ALARM_THRESHOLD_MM 100
#define MAX_DISTANCE_MM    4000

#define NUM_READINGS    5 // number of readings to take before returning median of distances (keep odd number >=3)

void lpuart_print_dec(LPUART_TypeDef *lpuart, uint32_t val) {
    char buffer[10];
    int i = 0;
    if (val == 0) {
        lpuartPutChar(lpuart, '0');
        return;
    }
    while (val > 0) {
        buffer[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) {
        lpuartPutChar(lpuart, buffer[--i]);
    }
}

/**
 * @brief Measures distance and returns millimeters for higher precision
 */
uint32_t sonar_read_mm(void) {
    // 10-microsecond HIGH pulse
    GPIO2->PSOR = (1 << TRIG_PIN);
    sysctrDelay_us(10);
    GPIO2->PCOR = (1 << TRIG_PIN);

    uint64_t timeout_start = sysctrGetTicks();
    uint64_t timeout_limit = (sysctrGetFreq() / 1000) * 24; // 24ms timeout
    
    while (!(GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sysctrGetTicks() - timeout_start) > timeout_limit) return 0xFFFFFFFF; 
    }

    uint64_t start_time = sysctrGetTicks();

    // Wait for Echo pin to go LOW
    while ((GPIO2->PDIR & (1 << ECHO_PIN))) {
        if ((sysctrGetTicks() - start_time) > timeout_limit) return 0xFFFFFFFF; 
    }

    // Calculate distance
    uint64_t end_time = sysctrGetTicks();
    uint64_t total_ticks = end_time - start_time;
    uint32_t duration_us = (total_ticks * 1000000ULL) / sysctrGetFreq();

    return (duration_us * 10) / 58;
}

/**
 * @brief Takes {NUM_READINGS} rapid readings and returns the median to eliminate noise/outliers
 */
uint32_t sonar_read_filtered_mm(void) {
    uint32_t readings[NUM_READINGS];
    
    // Take {NUM_READINGS} quick samples
    for(int i = 0; i < NUM_READINGS; i++) {
        readings[i] = sonar_read_mm();
        sysctrDelay_ms(10); // Wait 10ms between pings so the previous sound wave can die out
    }
    
    // Simple Bubble Sort to order the {NUM_READINGS} readings
    for (int i = 0; i < NUM_READINGS-1; i++) {
        for (int j = 0; j < NUM_READINGS-1-i; j++) {
            if (readings[j] > readings[j + 1]) {
                uint32_t temp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = temp;
            }
        }
    }
    
    // mean of the 3 middles
    uint32_t meanOfMiddles = (readings[NUM_READINGS/2 -1]+readings[NUM_READINGS/2]+readings[NUM_READINGS/2+1])/3;
    return meanOfMiddles;
}

int main() {
    lpuartPrintString(LPUART1, "\r\nHC-SR04 ultrasonic distance:\r\n");
    lpuartPrintString(LPUART1, " Press Ctrl+C to exit.\r\n\r\n");

    GPIO2->PDDR |= (1 << RED_LED); // output mode
    GPIO2->PCOR = (1 << RED_LED); // clear

    GPIO2->PDDR |= (1 << TRIG_PIN); // output mode
    GPIO2->PCOR = (1 << TRIG_PIN); // clear

    GPIO2->PDDR &= ~(1 << ECHO_PIN); // input mode (default)

    uint8_t led_state = 0;

    while (1) {
        char c = lpuartGetCharNonBlocking(LPUART1);
        if (c == 0x03) {  // Ctrl+C ASCII code
            lpuartPrintString(LPUART1, "\r\n[!] Ctrl+C caught! Shutting down Sonar...\r\n");
            GPIO2->PCOR = (1 << RED_LED); // clear
            GPIO2->PCOR = (1 << TRIG_PIN); // clear
            break; 
        }

        uint32_t distance_mm = sonar_read_filtered_mm(); // get mean of 3 middle readings for better accuracy

        if (distance_mm == 0xFFFFFFFF || distance_mm > MAX_DISTANCE_MM) { // out of range or timeout
            lpuartPrintString(LPUART1, "Distance: > 400.0 cm\r\n");
            GPIO2->PCOR = (1 << RED_LED); // clear
            led_state = 0;
        } else {
            lpuartPrintString(LPUART1, "Distance: ");
            lpuart_print_dec(LPUART1, distance_mm / 10); // centimeters
            lpuartPutChar(LPUART1, '.');
            lpuart_print_dec(LPUART1, distance_mm % 10); // millimeter
            lpuartPrintString(LPUART1, " cm\r\n");

            /* Proximity Blink Logic (< 10 cm) */
            if (distance_mm < ALARM_THRESHOLD_MM) {
                led_state = !led_state; // flip state
                if (led_state) {
                    GPIO2->PSOR = (1 << RED_LED); // led on
                } else {
                    GPIO2->PCOR = (1 << RED_LED); // led off
                }   
            } else {
                GPIO2->PCOR = (1 << RED_LED); // Force LED OFF
                led_state = 0;
            }
        }
    }

    return 0; 
}
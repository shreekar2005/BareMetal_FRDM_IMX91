// Task_Name : Sonar Proximity
#include <stdint.h>
#include "SYS_CTR.h"
#include "GPIO.h"
#include "../include/multitasking.h"
#include "../include/stdio.h"

/* also used in main.c hardware_init function */
#define TRIG_PIN  2  /* GPIO2_02 */
#define ECHO_PIN  3  /* GPIO2_03 */
#define RED_LED   13 /* GPIO2_13 */ 

/* Using millimeters for 1-decimal precision */
#define ALARM_THRESHOLD_MM 100  /* 10.0 cm */
#define MAX_DISTANCE_MM    4000 /* 400.0 cm */

#define NUM_READINGS 5 /* number of readings to take before returning median */

/**
 * @brief Measures distance and returns millimeters for higher precision
 */
static uint32_t sonar_read_mm(void) {
    /* ENTER CRITICAL SECTION: 
       Lock the OS so a 20ms context switch doesn't interrupt our microsecond math! */
    os_stop_scheduling();

    /* 1. Send mandatory 10-microsecond HIGH pulse using HAL */
    gpioWrite(GPIO2, TRIG_PIN, HIGH);
    sysctrDelay_us(10);
    gpioWrite(GPIO2, TRIG_PIN, LOW);

    /* 2. Wait for Echo pin to go HIGH */
    uint64_t timeout_start = sysctrGetTicks();
    uint64_t timeout_limit = (sysctrGetFreq() / 1000) * 24; // 24ms max range
    
    while (gpioRead(GPIO2, ECHO_PIN) == LOW) {
        if ((sysctrGetTicks() - timeout_start) > timeout_limit) {
            os_start_scheduling(); // Always unlock before returning!
            return 0xFFFFFFFF; 
        }
    }

    /* 3. Record start time */
    uint64_t start_time = sysctrGetTicks();

    /* 4. Wait for Echo pin to go LOW */
    while (gpioRead(GPIO2, ECHO_PIN) == HIGH) {
        if ((sysctrGetTicks() - start_time) > timeout_limit) {
            os_start_scheduling(); // Always unlock before returning!
            return 0xFFFFFFFF; 
        }
    }

    /* 5. Calculate precise distance */
    uint64_t end_time = sysctrGetTicks();
    
    /* EXIT CRITICAL SECTION: Safe to context switch again */
    os_start_scheduling();

    uint64_t total_ticks = end_time - start_time;
    uint32_t duration_us = (total_ticks * 1000000ULL) / sysctrGetFreq();

    /* duration_us / 58 = cm. Multiplying by 10 first gives us millimeters! */
    return (duration_us * 10) / 58;
}

/**
 * @brief Takes rapid readings and returns the median to eliminate noise/outliers
 */
static uint32_t sonar_read_filtered_mm(void) {
    uint32_t readings[NUM_READINGS];
    
    /* Take samples */
    for(int i = 0; i < NUM_READINGS; i++) {
        readings[i] = sonar_read_mm();
        
        /* RTOS Yield: Let other threads run while waiting for the sound to die out */
        thread_sleep(10); 
    }
    
    /* Simple Bubble Sort to order the readings from smallest to largest */
    for (int i = 0; i < NUM_READINGS-1; i++) {
        for (int j = 0; j < NUM_READINGS-1-i; j++) {
            if (readings[j] > readings[j + 1]) {
                uint32_t temp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = temp;
            }
        }
    }
    
    /* The outlier spikes are now at the ends of the array. Return the middle average! */
    uint32_t minOfMiddles = (readings[NUM_READINGS/2 - 1] + 
                             readings[NUM_READINGS/2] + 
                             readings[NUM_READINGS/2 + 1]) / 3;
    return minOfMiddles;
}

/**
 * @brief Main RTOS Thread Entrypoint for Sonar Task
 */
void sonar_thread(void* arg) {
    uint8_t led_state = 0; 

    /* The OS handles Ctrl+C automatically, so we just use an infinite loop */
    while (1) {
        uint32_t distance_mm = sonar_read_filtered_mm();

        /* If out of bounds or timed out */
        if (distance_mm == 0xFFFFFFFF || distance_mm > MAX_DISTANCE_MM) {
            print_dbg("[SONAR-Thread] Distance: > 400.0 cm\n");
            gpioWrite(GPIO2, RED_LED, LOW); 
            led_state = 0;
        } else {
            /* Print decimal formatted output directly using print_dbg */
            uint32_t cm_whole = distance_mm / 10;
            uint32_t cm_frac = distance_mm % 10;
            print_dbg("[SONAR-Thread] Distance: %d.%d cm\n", cm_whole, cm_frac);

            /* Proximity Blink Logic (< 10 cm) */
            if (distance_mm < ALARM_THRESHOLD_MM) {
                led_state = !led_state; /* Flip the bit */
                if (led_state) {
                    gpioWrite(GPIO2, RED_LED, HIGH); 
                } else {
                    gpioWrite(GPIO2, RED_LED, LOW); 
                }   
            } else {
                gpioWrite(GPIO2, RED_LED, LOW); 
                led_state = 0;
            }
        }
        
        /* Sleep for 100ms before next reading so we don't spam the CLI */
        thread_sleep(100);
    }
}
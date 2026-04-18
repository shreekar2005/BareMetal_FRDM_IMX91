// Task_Name : Sonar Proximity
#include <stdint.h>
#include "SYS_CTR.h"
#include "GPIO.h"
#include "../include/multitasking.h"
#include "../include/stdio.h"

extern int ledblink_frequency_hz; // defined in led.c

#define TRIG_PIN  2  // GPIO2_02
#define ECHO_PIN  3  // GPIO2_03

#define THRESHOLD_MM        500 // 50 cm threshold for changin frequency
#define MAX_DISTANCE_MM     4000

#define NUM_READINGS    5 // number of readings to take before returning median of distances (keep odd number >=3)

/**
 * @brief Measures distance and returns millimeters for higher precision
 */
static uint32_t sonar_read_mm(void) {
    os_stop_scheduling(); // stopping scheduling for precise timing control

    // 10-microsecond HIGH pulse
    gpioWrite(GPIO2, TRIG_PIN, HIGH);
    sysctrDelay_us(10);
    gpioWrite(GPIO2, TRIG_PIN, LOW);

    uint64_t timeout_start = sysctrGetTicks();
    uint64_t timeout_limit = (sysctrGetFreq() / 1000) * 24; // 24ms timeout
    
    while (gpioRead(GPIO2, ECHO_PIN) == LOW) {
        if ((sysctrGetTicks() - timeout_start) > timeout_limit) {
            os_start_scheduling(); // start scheduling before returning
            return 0xFFFFFFFF; 
        }
    }

    uint64_t start_time = sysctrGetTicks();
    // Wait for Echo pin to go LOW
    while (gpioRead(GPIO2, ECHO_PIN) == HIGH) {
        if ((sysctrGetTicks() - start_time) > timeout_limit) {
            os_start_scheduling(); // start scheduling before returning
            return 0xFFFFFFFF; 
        }
    }

    uint64_t end_time = sysctrGetTicks();
    
    os_start_scheduling(); // start scheduling

    uint64_t total_ticks = end_time - start_time;
    uint32_t duration_us = (total_ticks * 1000000ULL) / sysctrGetFreq();

    return (duration_us * 10) / 58; // distance in mm
}

/**
 * @brief Takes rapid readings and returns the median to eliminate noise/outliers
 */
static uint32_t sonar_read_filtered_mm(void) {
    uint32_t readings[NUM_READINGS];
    
    // taking samples
    for(int i = 0; i < NUM_READINGS; i++) {
        readings[i] = sonar_read_mm();
        thread_sleep(10); 
    }
    
    // sorting samples
    for (int i = 0; i < NUM_READINGS-1; i++) {
        for (int j = 0; j < NUM_READINGS-1-i; j++) {
            if (readings[j] > readings[j + 1]) {
                uint32_t temp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = temp;
            }
        }
    }
    
    // taking mean of middle three
    uint32_t meanOfMiddles = (readings[NUM_READINGS/2 - 1] + 
                             readings[NUM_READINGS/2] + 
                             readings[NUM_READINGS/2 + 1]) / 3;
    return meanOfMiddles;
}

/**
 * @brief Entrypoint for Sonar Task
 */
void sonar_thread(void* arg) {
    while (1) {
        uint32_t distance_mm = sonar_read_filtered_mm();

        if (distance_mm == 0xFFFFFFFF || distance_mm > MAX_DISTANCE_MM) {
            print_dbg("[SONAR-Thread] Distance: > 400.0 cm\n");
            ledblink_frequency_hz = 1; // Reset to slow baseline
        } else {
            uint32_t cm_whole = distance_mm / 10;
            uint32_t cm_frac = distance_mm % 10;
            print_dbg("[SONAR-Thread] Distance: %d.%d cm\n", cm_whole, cm_frac);

            // Set frequency based on distance (linear)
            if (distance_mm <= THRESHOLD_MM) {
                uint32_t safe_dist = (distance_mm == 0) ? 1 : distance_mm; // Prevent division by zero
                ledblink_frequency_hz = 100 - ((distance_mm * 99) / THRESHOLD_MM); // Max 100 Hz, Min 1 Hz
            } else {
                ledblink_frequency_hz = 1; // Object is outside threshold, blink slowly
            }
        }
        
        // sleep between 2 outputs
        thread_sleep(100);
    }
}
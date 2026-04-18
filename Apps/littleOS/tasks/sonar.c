// Task_Name : Sonar Proximity
#include <stdint.h>
#include "SYS_CTR.h"
#include "GPIO.h"
#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/common_macros.h"

extern int ledblink_delay_ms; // defined in led.c

#define THRESHOLD_MM    250 // 25 cm threshold for changing delay of led blink
#define MAX_DISTANCE_MM 4000 // 400 cm max distance (beyond this we just say > 400 cm)
#define NUM_READINGS    5 // number of readings to take before returning median of distances (keep odd number >=3)

/**
 * @brief Measures distance and returns millimeters for higher precision
 */
static uint32_t sonar_read_mm(void);

/**
 * @brief Takes rapid readings and returns the median to eliminate noise/outliers
 */
static uint32_t sonar_read_filtered_mm(void);

/**
 * @brief Entrypoint for Sonar Task
 */
void sonar_thread(void* arg) {
    print_dbg("\n");
    while (1) {
        uint32_t distance_mm = sonar_read_filtered_mm();

        if (distance_mm == 0xFFFFFFFF || distance_mm > MAX_DISTANCE_MM) {
            print_dbg("[SONAR-Thread] Distance: > 400.0 cm\n");
            ledblink_delay_ms = 2000; // Reset to slow baseline (2 seconds)
        } else {
            print_dbg("[SONAR-Thread] Distance: %.2f cm\n", distance_mm / 10.0);

            // Set delay based on distance 
            if (distance_mm <= THRESHOLD_MM) {
                ledblink_delay_ms = (distance_mm * 1000) / THRESHOLD_MM;
                
                // Safety clamp: prevent a true 0ms sleep which could lock the LED thread
                if (ledblink_delay_ms < 5) {
                    ledblink_delay_ms = 5;
                }
            } else {
                ledblink_delay_ms = 2000; // Object is outside threshold, blink very slowly
            }
        }
        
        // sleep between 2 outputs
        thread_sleep(100);
    }
}


static uint32_t sonar_read_mm(void) {
    os_stop_scheduling(); // stopping scheduling for precise timing control

    // 10-microsecond HIGH pulse
    gpioWrite(GPIO2, ULTRASONIC_TRIG_PIN, HIGH);
    sysctrDelay_us(10);
    gpioWrite(GPIO2, ULTRASONIC_TRIG_PIN, LOW);

    uint64_t timeout_start = sysctrGetTicks();
    uint64_t timeout_limit = (sysctrGetFreq() / 1000) * 24; // 24ms timeout
    
    while (gpioRead(GPIO2, ULTRASONIC_ECHO_PIN) == LOW) {
        if ((sysctrGetTicks() - timeout_start) > timeout_limit) {
            os_start_scheduling(); // start scheduling before returning
            return 0xFFFFFFFF; 
        }
    }

    uint64_t start_time = sysctrGetTicks();
    // Wait for Echo pin to go LOW
    while (gpioRead(GPIO2, ULTRASONIC_ECHO_PIN) == HIGH) {
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
// Task_Name : LED Blink

#include "include/multitasking.h"
#include "include/stdio.h"
#include "GPIO.h"

#define LED_PIN (1 << 4)

void ledblink_thread(void* arg) {
    printf("\nLED Blink Thread Started!\n");
    GPIO2->PSOR = LED_PIN; 
    os_sleep_ms(300);        
    GPIO2->PCOR = LED_PIN;
    printf("LED Blink Thread Ended!\n");
}
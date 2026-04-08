// Task_Name : LED Blink

#include "include/multitasking.h"
#include "include/stdio.h"
#include "GPIO.h"

void ledblink_thread(void* arg) {
    printdbg("\nLED Blink Thread Started!\n");
    digitalWrite(GPIO2, 4, HIGH); // GPIO2->PSOR = (1 << 4); 
    os_sleep_ms(300);        
    digitalWrite(GPIO2, 4, LOW); // GPIO2->PCOR = (1 << 4);
    printdbg("LED Blink Thread Ended!\n");
}
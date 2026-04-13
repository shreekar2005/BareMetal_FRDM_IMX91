// Task_Name : LED Blink

#include "GPIO.h"
#include "include/multitasking.h"
#include "include/stdio.h"

void ledblink_thread(void* arg) {
    gpioWrite(GPIO2, 4, HIGH); // GPIO2->PSOR = (1 << 4); 
    thread_sleep(300);        
    gpioWrite(GPIO2, 4, LOW); // GPIO2->PCOR = (1 << 4);
    thread_sleep(300);
    gpioWrite(GPIO2, 4, HIGH); // GPIO2->PSOR = (1 << 4); 
    thread_sleep(300);        
    gpioWrite(GPIO2, 4, LOW); // GPIO2->PCOR = (1 << 4);
}
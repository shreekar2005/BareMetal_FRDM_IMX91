// Task_Name : Print X's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100X_thread(void* arg) {
    for(int i = 0; i < 100; i++) {
        printdbg("X");
        os_sleep_ms(50);
    }
}
// Task_Name : Print 100 X's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100X_thread(void* arg) {
    for(int i = 0; i < 100; i++) {
        printf("X");
        os_sleep_ms(50);
    }
}
// Task_Name : Print 100 o's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100o_thread(void* arg) {
    for(int i = 0; i < 100; i++) {
        printf("o");
        os_sleep_ms(50);
    }
}
// Task_Name : Print o's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100o_thread(void* arg) {
    for(int i = 0; i < 100; i++) {
        printdbg("o");
        os_sleep_ms(50);
    }
}
// Task_Name : Print X's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100X_thread(void* arg) {
    print_dbg("\n");
    for(int i = 0; i < 100; i++) {
        print_dbg("X");
        thread_sleep(50);
    }
}
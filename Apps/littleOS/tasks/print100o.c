// Task_Name : Print o's

#include "include/multitasking.h"
#include "include/stdio.h"

void print100o_thread(void* arg) {
    print_dbg("\n");
    for(int i = 0; i < 100; i++) {
        print_dbg("o");
        thread_sleep(50);
    }
}
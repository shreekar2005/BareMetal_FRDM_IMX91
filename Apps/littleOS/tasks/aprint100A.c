// Task_Name : AtomicPrint A's

#include "include/multitasking.h"
#include "include/stdio.h"

void aprint100A_thread(void* arg) {
    scheduling_stop();
    for(int i = 0; i < 100; i++) {
        print_dbg("A");
        thread_sleep(50); // Auto-downgrades to busy-wait safely!
    }
    scheduling_start();
}

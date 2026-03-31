// Task_Name : Atomic Print 100 A's

#include "include/multitasking.h"
#include "include/stdio.h"

void aprint100A_thread(void* arg) {
    os_stop_scheduling();
    for(int i = 0; i < 100; i++) {
        printf("A");
        os_sleep_ms(50); // Auto-downgrades to busy-wait safely!
    }
    os_start_scheduling();
}

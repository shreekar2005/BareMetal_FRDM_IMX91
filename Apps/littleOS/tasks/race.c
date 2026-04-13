// Task_Name : Race Condition

#include <stddef.h>
#include "include/multitasking.h"
#include "include/stdio.h"

volatile int shared_counter = 0;

void func_inc(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        shared_counter++; 
    }
}

void race_thread(void* arg) {
    print_dbg("\n[RACE-Thread] Starting race condition test...\n");
    shared_counter = 0; // Reset in case we run the command multiple times
    
    int t1 = os_create_thread("Race-Slave1", func_inc, NULL);
    int t2 = os_create_thread("Race-Slave2", func_inc, NULL);
    
    os_thread_start(t1);
    os_thread_start(t2);
    
    os_join_thread(t1);
    os_join_thread(t2);
    
    print_dbg("\n[RACE-Thread] Expected: 20000000");
    print_dbg("\n[RACE-Thread] Actual:   %d\n", shared_counter);
    
    int data_loss = 20000000 - shared_counter;
    print_dbg("[RACE-Thread] Context switches caused a loss of %d increments!", data_loss);
}
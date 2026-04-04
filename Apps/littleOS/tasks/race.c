// Task_Name : Race Condition
#include "include/multitasking.h"
#include "include/stdio.h"
#include <stddef.h>

volatile int shared_counter = 0;

void func_inc(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        shared_counter++; 
    }
}

void race_thread(void* arg) {
    printf("\r\n[RACE] Starting race condition test...\r\n");
    shared_counter = 0; // Reset in case we run the command multiple times
    
    int t1 = os_create_thread("Race1", func_inc, NULL);
    int t2 = os_create_thread("Race2", func_inc, NULL);
    
    os_thread_start(t1);
    os_thread_start(t2);
    
    os_join_thread(t1);
    os_join_thread(t2);
    
    printf("\r\n[RACE] Expected: 20000000");
    printf("\r\n[RACE] Actual:   %d\r\n", shared_counter);
    
    int data_loss = 20000000 - shared_counter;
    printf("[RACE] Context switches caused a loss of %d increments!\r\n", data_loss);
}
// Task_Name : Race Condition

#include <stddef.h>
#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/shared_locks.h"
#include "SYS_CTR.h"

volatile int shared_counter = 0;
os_mutex_t race_mutex;

void func_inc(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        shared_counter++; 
    }
}
void func_inc_mutex(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        os_mutex_lock(&race_mutex);
        shared_counter++; 
        os_mutex_unlock(&race_mutex);
    }
}

void race_thread(void* arg) {
    print_dbg("\n");
    int startTicks;
    int endTicks;
    int timeElapsed_ms;
    int loss;
    
    static int t1 = -1;
    static int t2 = -1;
    static int t3 = -1;
    static int t4 = -1;

    if (t1 == -1) {
        t1 = os_create_thread("Race-Slave1", func_inc, NULL);
        t2 = os_create_thread("Race-Slave2", func_inc, NULL);
        t3 = os_create_thread("Race-Slave3", func_inc_mutex, NULL);
        t4 = os_create_thread("Race-Slave4", func_inc_mutex, NULL);
    }

    print_dbg("[RACE-Thread] Starting race condition test...\n");
    print_dbg("[RACE-Thread] Expected: 20000000\n");

    shared_counter = 0;
    startTicks = sysctrGetTicks();
    
    os_thread_start(t1);
    os_thread_start(t2);
    os_join_thread(t1);
    os_join_thread(t2);
    
    endTicks = sysctrGetTicks();
    timeElapsed_ms = ((endTicks - startTicks) * 1000) / sysctrGetFreq();
    loss= 20000000 - shared_counter;
    print_dbg("[RACE-Thread] By 2 threads without mutex: %d, elapsed_ms=%d, LOSS=%d\n", shared_counter, timeElapsed_ms, loss);

    shared_counter = 0;
    startTicks = sysctrGetTicks();
    
    // Revive the mutex threads
    os_thread_start(t3);
    os_thread_start(t4);
    os_join_thread(t3);
    os_join_thread(t4);
    
    endTicks = sysctrGetTicks();
    timeElapsed_ms = ((endTicks - startTicks) * 1000) / sysctrGetFreq();
    loss= 20000000 - shared_counter;
    print_dbg("[RACE-Thread] By 2 threads with mutex: %d, elapsed_ms=%d, LOSS=%d\n", shared_counter, timeElapsed_ms, loss);
}
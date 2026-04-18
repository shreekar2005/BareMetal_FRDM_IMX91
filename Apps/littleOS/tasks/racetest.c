// Task_Name : Race Condition

#include <stddef.h>
#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/shared_locks.h"
#include "SYS_CTR.h"

volatile int shared_counter = 0;
os_mutex_t race_mutex = OS_MUTEX_INITIALIZER;

void func_inc(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        shared_counter++; 
    }
}
void func_inc_mutex(void* arg) {
    for(volatile int i = 0; i < 10000000; i++) {
        mutex_lock(&race_mutex);
        shared_counter++; 
        mutex_unlock(&race_mutex);
    }
}

void racetest_thread(void* arg) {
    int loss;
    static int t1 = -1; // making this static so that we only create the threads once and can re-run test multiple times
    static int t2 = -1;
    static int t3 = -1;
    static int t4 = -1;

    if (t1 == -1) {
        t1 = thread_create("Race-Slave1", func_inc, NULL);
        t2 = thread_create("Race-Slave2", func_inc, NULL);
        t3 = thread_create("Race-Slave3", func_inc_mutex, NULL);
        t4 = thread_create("Race-Slave4", func_inc_mutex, NULL);
    }

    print_dbg("[RACE-Thread] Starting race condition test...\n");
    print_dbg("[RACE-Thread] Expected: 20000000\n");

    shared_counter = 0;
    thread_start(t1);
    thread_start(t2);
    thread_join(t1);
    thread_join(t2);
    
    loss= 20000000 - shared_counter;
    print_dbg("[RACE-Thread] Outcome : %d, LOSS=%d - by two threads without mutex\n", shared_counter, loss);

    shared_counter = 0;
    thread_start(t3);
    thread_start(t4);
    thread_join(t3);
    thread_join(t4);
    
    loss= 20000000 - shared_counter;
    print_dbg("[RACE-Thread] Outcome : %d, LOSS=%d - by two threads with mutex\n", shared_counter, loss);
}
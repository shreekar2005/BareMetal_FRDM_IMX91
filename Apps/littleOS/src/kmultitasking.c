#include "kmultitasking.h"

Thread threads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;

void os_yield(void) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (1));
    __asm__ volatile("nop");
    __asm__ volatile("nop");
    __asm__ volatile("nop");
}

void os_thread_exit(void) {
    threads[current_thread].active = false; // Thread goes dead/asleep
    os_yield(); 
    while(1) { __asm__ volatile("wfi"); }
}

void os_init_scheduler(void) {
    num_threads = 0;
    current_thread = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].active = false;
    }
}

void os_suspend_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].active = false;
    }
}

// The explicit trigger!
void os_thread_start(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].active = true;
    }
}

int os_create_thread(void (*entrypoint)(void*), void* arg) {
    if (num_threads >= MAX_THREADS) return -1;

    Thread* t = &threads[num_threads];
    t->entrypoint = entrypoint;
    t->arg = arg;
    
    // Per your design: Threads are born DORMANT.
    t->active = false; 

    t->cpustate_ptr = (CPUState*)(t->stack + sizeof(t->stack) - sizeof(CPUState));

    for (int i = 0; i < 30; i++) {
        t->cpustate_ptr->x[i] = 0;
    }

    t->cpustate_ptr->x[0] = (uint64_t)arg;
    t->cpustate_ptr->lr = (uint64_t)&os_thread_exit;
    t->cpustate_ptr->pc = (uint64_t)entrypoint;
    t->cpustate_ptr->cpsr = 0x009; 
    t->cpustate_ptr->sp = (uint64_t)t->cpustate_ptr;

    num_threads++;
    return num_threads - 1; 
}

CPUState* schedule_tick(CPUState* current_cpustate_ptr) {
    if (num_threads == 0) return current_cpustate_ptr;

    // Save current thread (if it exists)
    if (current_thread >= 0) {
        threads[current_thread].cpustate_ptr = current_cpustate_ptr;
    }

    int starting_thread = current_thread;
    do {
        current_thread++;
        if (current_thread >= num_threads) {
            current_thread = 0;
        }
        
        // Your exact logic: only schedule if active is true!
        if (threads[current_thread].active) {
            return threads[current_thread].cpustate_ptr;
        }
    } while (current_thread != starting_thread);

    return current_cpustate_ptr;
}
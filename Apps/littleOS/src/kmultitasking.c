#include "kmultitasking.h"

Thread threads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;

void os_thread_exit(void) {
    // Mark this thread as dead
    threads[current_thread].active = false;
    
    // Hand the CPU to the next thread forever
    while(1) {
        os_yield(); 
    }
}

void os_init_scheduler(void) {
    num_threads = 0;
    current_thread = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].active = false;
    }
}

bool os_create_thread(void (*entrypoint)(void*), void* arg) {
    if (num_threads >= MAX_THREADS) return false;

    Thread* t = &threads[num_threads];
    t->entrypoint = entrypoint;
    t->arg = arg;
    t->active = true;

    t->cpustate = (CPUState*)(t->stack + sizeof(t->stack) - sizeof(CPUState));

    for (int i = 0; i < 30; i++) {
        t->cpustate->x[i] = 0;
    }

    t->cpustate->x[0] = (uint64_t)arg;
    t->cpustate->lr = (uint64_t)&os_thread_exit;
    t->cpustate->pc = (uint64_t)entrypoint;
    t->cpustate->cpsr = 0x009; // EL2h mode
    t->cpustate->sp = (uint64_t)t->cpustate;

    num_threads++;
    return true;
}

CPUState* schedule_tick(CPUState* current_state) {
    if (num_threads == 0) return current_state;

    if (current_thread >= 0) {
        threads[current_thread].cpustate = current_state;
    }

    int starting_thread = current_thread;
    do {
        current_thread++;
        if (current_thread >= num_threads) {
            current_thread = 0;
        }
        if (threads[current_thread].active) {
            return threads[current_thread].cpustate;
        }
    } while (current_thread != starting_thread);

    return current_state;
}
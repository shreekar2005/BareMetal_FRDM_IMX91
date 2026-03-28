#include "include/multitasking.h"

Thread threads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;
bool scheduling_enabled = true; 
enum SchedAlgo current_algo = SCHED_RR; 

// external hardware frequency getter for absolute deadline math
extern uint32_t sys_ctr_get_freq(void); 

void os_yield(void) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (1));
    __asm__ volatile("nop");
    __asm__ volatile("nop");
    __asm__ volatile("nop");
}

void os_thread_exit(void) {
    threads[current_thread].active = false; 
    os_yield(); 
    while(1) { __asm__ volatile("wfi"); }
}

void os_init_scheduler(void) {
    num_threads = 0;
    current_thread = -1;
    scheduling_enabled = true;
    current_algo = SCHED_RR;
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].active = false;
        threads[i].priority = 128; // safe middle default
        threads[i].deadline_offset_ms = 10000; // 10 sec default
    }
}

void os_set_scheduling_algo(enum SchedAlgo algo) {
    current_algo = algo;
}

void os_stop_scheduling(void) {
    scheduling_enabled = false;
}

void os_start_scheduling(void) {
    scheduling_enabled = true;
}

void os_suspend_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].active = false;
    }
}

void os_set_thread_rtos(int thread_id, int priority, uint32_t deadline_ms) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].priority = priority;
        threads[thread_id].deadline_offset_ms = deadline_ms;
    }
}

void os_thread_start(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        Thread* t = &threads[thread_id];
        
        // calculate absolute hardware deadline for edf scheduling
        uint64_t current_ticks;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r" (current_ticks));
        uint32_t freq = sys_ctr_get_freq();
        uint64_t ticks_to_add = ((uint64_t)freq * t->deadline_offset_ms) / 1000ULL;
        t->absolute_deadline_tick = current_ticks + ticks_to_add;

        // rebuild physical stack if the thread was dead
        if (!t->active) {
            t->cpustate_ptr = (CPUState*)(t->stack + sizeof(t->stack) - sizeof(CPUState));
            for (int i = 0; i < 30; i++) {
                t->cpustate_ptr->x[i] = 0;
            }
            t->cpustate_ptr->x[0] = (uint64_t)t->arg;
            t->cpustate_ptr->lr = (uint64_t)&os_thread_exit;
            t->cpustate_ptr->pc = (uint64_t)t->entrypoint;
            t->cpustate_ptr->cpsr = 0x009; 
            t->cpustate_ptr->sp = (uint64_t)t->cpustate_ptr;
        }
        
        t->active = true;
    }
}

int os_create_thread(void (*entrypoint)(void*), void* arg) {
    if (num_threads >= MAX_THREADS) return -1;

    Thread* t = &threads[num_threads];
    t->entrypoint = entrypoint;
    t->arg = arg;
    t->active = false; 
    t->priority = 128;
    t->deadline_offset_ms = 10000;

    os_thread_start(num_threads);
    t->active = false; 

    num_threads++;
    return num_threads - 1; 
}

CPUState* schedule_tick(CPUState* current_cpustate_ptr) {
    if (num_threads == 0 || !scheduling_enabled) {
        return current_cpustate_ptr;
    }

    if (current_thread >= 0) {
        threads[current_thread].cpustate_ptr = current_cpustate_ptr;
    }

    if (current_algo == SCHED_RR) {
        int starting_thread = current_thread;
        do {
            current_thread++;
            if (current_thread >= num_threads) current_thread = 0;
            if (threads[current_thread].active) return threads[current_thread].cpustate_ptr;
        } while (current_thread != starting_thread);
    } 
    else if (current_algo == SCHED_PRIORITY) {
        int best_thread = -1;
        int highest_pri = 999999; // lower number is higher priority
        
        for (int i = 0; i < num_threads; i++) {
            if (threads[i].active && threads[i].priority < highest_pri) {
                highest_pri = threads[i].priority;
                best_thread = i;
            }
        }
        
        if (best_thread != -1) {
            current_thread = best_thread;
            return threads[current_thread].cpustate_ptr;
        }
    }
    else if (current_algo == SCHED_EDF) {
        int best_thread = -1;
        uint64_t earliest_deadline = 0xFFFFFFFFFFFFFFFFULL;
        
        for (int i = 0; i < num_threads; i++) {
            if (threads[i].active && threads[i].absolute_deadline_tick < earliest_deadline) {
                earliest_deadline = threads[i].absolute_deadline_tick;
                best_thread = i;
            }
        }
        
        if (best_thread != -1) {
            current_thread = best_thread;
            return threads[current_thread].cpustate_ptr;
        }
    }

    return current_cpustate_ptr; // fallback if no suitable thread found
}
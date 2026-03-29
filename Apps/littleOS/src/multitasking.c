#include "include/multitasking.h"
#include "include/string.h"

Thread threads[MAX_THREADS];
int num_threads = 0;
int current_thread = -1;
bool scheduling_enabled = true; 
enum SchedAlgo current_algo = SCHED_RR; 

extern uint32_t sys_ctr_get_freq(void); 

void os_yield(void) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (1));
    __asm__ volatile("nop");
    __asm__ volatile("nop");
    __asm__ volatile("nop");
}

// Don't forget to declare the delay function at the top of the file if it isn't there!
extern void sys_ctr_delay_ms(uint32_t ms);

void os_sleep_ms(uint32_t ms) {
    if (current_thread < 0) return;
    
    // Safety net for atomic blocks
    if (!scheduling_enabled) {
        sys_ctr_delay_ms(ms);
        return;
    }
    
    uint64_t current_ticks;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (current_ticks));
    uint32_t freq = sys_ctr_get_freq();
    uint64_t sleep_ticks = ((uint64_t)freq * ms) / 1000ULL;
    
    threads[current_thread].wakeup_tick = current_ticks + sleep_ticks;
    threads[current_thread].sleeping = true;
    threads[current_thread].active = false; 
    
    os_yield(); 
    
    while(threads[current_thread].sleeping) {
        __asm__ volatile("nop");
    }
}

void os_thread_exit(void) {
    uint64_t end_ticks;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (end_ticks));
    uint32_t freq = sys_ctr_get_freq();
    
    threads[current_thread].last_exec_time_ms = ((end_ticks - threads[current_thread].last_start_tick) * 1000ULL) / freq;
    
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
        threads[i].priority = 128; 
        threads[i].deadline_offset_ms = -1; 
        threads[i].period_ms = 0; 
        threads[i].executions_target = 0;
        threads[i].executions_done = 0;
        threads[i].last_start_tick = 0;
        threads[i].last_exec_time_ms = 0;
        threads[i].sleeping = false;
        threads[i].wakeup_tick = 0;
        my_strcpy(threads[i].name, "EMPTY");
    }
}

void os_set_scheduling_algo(enum SchedAlgo algo) { current_algo = algo; }
void os_stop_scheduling(void) { scheduling_enabled = false; }
void os_start_scheduling(void) { scheduling_enabled = true; }

void os_suspend_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].active = false;
    }
}

void os_kill_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        // Strip away all active states so the scheduler ignores it immediately
        threads[thread_id].active = false;
        threads[thread_id].sleeping = false;
        threads[thread_id].executions_target = 0;
        threads[thread_id].executions_done = 0;
    }
}

void os_set_thread_rtos(int thread_id, int priority, int deadline_ms, uint32_t period_ms, int exec_target) {
    if (thread_id >= 0 && thread_id < num_threads) {
        threads[thread_id].priority = priority;
        threads[thread_id].deadline_offset_ms = deadline_ms;
        threads[thread_id].period_ms = period_ms;
        threads[thread_id].executions_target = exec_target;
        threads[thread_id].executions_done = 0; 
    }
}

void os_thread_start(int thread_id) {
    if (thread_id >= 0 && thread_id < num_threads) {
        Thread* t = &threads[thread_id];
        
        uint64_t current_ticks;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r" (current_ticks));
        uint32_t freq = sys_ctr_get_freq();
        
        if (t->deadline_offset_ms == -1) {
            t->absolute_deadline_tick = 0xFFFFFFFFFFFFFFFFULL;
        } else {
            uint64_t deadline_ticks = ((uint64_t)freq * t->deadline_offset_ms) / 1000ULL;
            t->absolute_deadline_tick = current_ticks + deadline_ticks;
        }

        uint64_t period_ticks = ((uint64_t)freq * t->period_ms) / 1000ULL;
        t->next_period_tick = current_ticks + period_ticks;

        if (!t->active) {
            t->cpustate_ptr = (CPUState*)(t->stack + sizeof(t->stack) - sizeof(CPUState));
            for (int i = 0; i < 30; i++) t->cpustate_ptr->x[i] = 0;
            t->cpustate_ptr->x[0] = (uint64_t)t->arg;
            t->cpustate_ptr->lr = (uint64_t)&os_thread_exit;
            t->cpustate_ptr->pc = (uint64_t)t->entrypoint;
            t->cpustate_ptr->cpsr = 0x009; 
            t->cpustate_ptr->sp = (uint64_t)t->cpustate_ptr;
            
            t->last_start_tick = current_ticks;
            t->executions_done++;
        }
        t->active = true;
    }
}

int os_create_thread(const char* name, void (*entrypoint)(void*), void* arg) {
    if (num_threads >= MAX_THREADS) return -1;

    Thread* t = &threads[num_threads];
    my_strcpy(t->name, name);
    t->entrypoint = entrypoint;
    t->arg = arg;
    t->active = false; 
    t->priority = 128;
    t->deadline_offset_ms = -1;
    t->period_ms = 0;
    t->executions_target = 0;
    t->executions_done = 0;
    t->last_start_tick = 0;
    t->last_exec_time_ms = 0;
    t->sleeping = false;
    t->wakeup_tick = 0;

    os_thread_start(num_threads);
    t->active = false; 
    t->executions_done = 0; 

    num_threads++;
    return num_threads - 1; 
}

CPUState* schedule_tick(CPUState* current_cpustate_ptr) {
    if (num_threads == 0 || !scheduling_enabled) return current_cpustate_ptr;

    if (current_thread >= 0) {
        threads[current_thread].cpustate_ptr = current_cpustate_ptr;
    }

    uint64_t current_ticks;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (current_ticks));
    
    // The Wakeup & Revival Loop
    for (int i = 0; i < num_threads; i++) {
        // 1. Wake up sleeping threads (Without wiping their stack!)
        if (threads[i].sleeping) {
            if (current_ticks >= threads[i].wakeup_tick) {
                threads[i].sleeping = false;
                threads[i].active = true;
            }
        }
        // 2. Revive dead periodic tasks (Wipes their stack to start fresh)
        else if (!threads[i].active && i != current_thread) {
            if (threads[i].executions_target == -1 || threads[i].executions_done < threads[i].executions_target) {
                if (current_ticks >= threads[i].next_period_tick) {
                    os_thread_start(i); 
                }
            }
        }
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
        int highest_pri = 999999; 
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
            if (threads[i].active) {
                if (best_thread == -1 || threads[i].absolute_deadline_tick < earliest_deadline) {
                    earliest_deadline = threads[i].absolute_deadline_tick;
                    best_thread = i;
                }
            }
        }
        if (best_thread != -1) {
            current_thread = best_thread;
            return threads[current_thread].cpustate_ptr;
        }
    }

    return current_cpustate_ptr; 
}
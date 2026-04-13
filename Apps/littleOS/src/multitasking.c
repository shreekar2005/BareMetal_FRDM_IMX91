#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/string.h"
#include "include/stdio.h"

Thread threads[MAX_THREADS];
int numThreads = 0;
int currentThread_idx = -1;
bool isSchedulingEnabled = true; 
enum SchedAlgo currentSchedAlgo = SCHED_RR; 

const char* get_thread_state_name(enum ThreadState state) {
    switch (state) {
        case STATE_NEW: return "NEW";
        case STATE_READY: return "READY";
        case STATE_RUN: return "RUN";
        case STATE_WAIT_BLOCK: return "BLOCK";
        case STATE_TERMINATE: return "TERM";
        case STATE_SUSPEND_READY: return "S_READY";
        case STATE_SUSPEND_WAIT: return "S_WAIT";
        default: return "UNKNOWN";
    }
}

void os_yield(void) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (1));
    __asm__ volatile("nop");
    __asm__ volatile("nop");
    __asm__ volatile("nop");
}

void thread_sleep(uint32_t ms) {
    if (currentThread_idx < 0) return;
    
    // Safety net for atomic blocks
    if (!isSchedulingEnabled) {
        sysctrDelay_ms(ms);
        return;
    }
    
    uint64_t current_ticks;
    current_ticks = sysctrGetTicks();
    uint32_t freq = sysctrGetFreq();
    uint64_t sleep_ticks = ((uint64_t)freq * ms) / 1000ULL;
    
    threads[currentThread_idx].wakeupTick = current_ticks + sleep_ticks;
    threads[currentThread_idx].currentState = STATE_WAIT_BLOCK;
    
    os_yield(); 
    
    while(threads[currentThread_idx].currentState == STATE_WAIT_BLOCK) {
        __asm__ volatile("nop");
    }
}

void os_thread_exit(void) {
    uint64_t end_ticks;
    end_ticks=sysctrGetTicks();
    uint32_t freq = sysctrGetFreq();
    
    threads[currentThread_idx].lastTurnaroundTime_ms = ((end_ticks - threads[currentThread_idx].lastStartTick) * 1000ULL) / freq;
    
    threads[currentThread_idx].currentState = STATE_TERMINATE;
    os_yield(); 
    while(1) { __asm__ volatile("wfi"); }
}

void os_init_scheduler(void) {
    numThreads = 0;
    currentThread_idx = -1;
    isSchedulingEnabled = true;
    currentSchedAlgo = SCHED_RR;
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].currentState = STATE_NEW;
        threads[i].priority = 128; 
        threads[i].deadlineOffset_ms = -1; 
        threads[i].period_ms = 0; 
        threads[i].executionsTarget = 0;
        threads[i].executionsDone = 0;
        threads[i].lastStartTick = 0;
        threads[i].lastTurnaroundTime_ms = 0;
        threads[i].wakeupTick = 0;
        strcpy(threads[i].name, "EMPTY");
    }
}

void os_set_scheduling_algo(enum SchedAlgo algo) { currentSchedAlgo = algo; }
void os_stop_scheduling(void) { isSchedulingEnabled = false; }
void os_start_scheduling(void) { isSchedulingEnabled = true; }

void os_suspend_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < numThreads) {
        if (threads[thread_id].currentState == STATE_WAIT_BLOCK) {
            threads[thread_id].currentState = STATE_SUSPEND_WAIT;
        } else {
            threads[thread_id].currentState = STATE_SUSPEND_READY;
        }
    }
}

void os_kill_thread(int thread_id) {
    if (thread_id >= 0 && thread_id < numThreads) {
        // Strip away all active states so the scheduler ignores it immediately
        threads[thread_id].executionsTarget = 0;
        threads[thread_id].executionsDone = 0;
        threads[thread_id].currentState = STATE_TERMINATE;
    }
}

void os_set_thread_rtos(int thread_id, int priority, int deadline_ms, uint32_t period_ms, int exec_target) {
    if (thread_id >= 0 && thread_id < numThreads) {
        threads[thread_id].priority = priority;
        threads[thread_id].deadlineOffset_ms = deadline_ms;
        threads[thread_id].period_ms = period_ms;
        threads[thread_id].executionsTarget = exec_target;
        threads[thread_id].executionsDone = 0; 
    }
}

void os_thread_start(int thread_id) {
    if (thread_id >= 0 && thread_id < numThreads) {
        Thread* t = &threads[thread_id];
        
        uint64_t current_ticks;
        current_ticks = sysctrGetTicks();
        uint32_t freq = sysctrGetFreq();
        
        if (t->deadlineOffset_ms == -1) {
            t->absoluteDeadlineTick = 0xFFFFFFFFFFFFFFFFULL;
        } else {
            uint64_t deadline_ticks = ((uint64_t)freq * t->deadlineOffset_ms) / 1000ULL;
            t->absoluteDeadlineTick = current_ticks + deadline_ticks;
        }

        uint64_t period_ticks = ((uint64_t)freq * t->period_ms) / 1000ULL;
        t->nextPeriodTick = current_ticks + period_ticks;

        if (t->currentState == STATE_NEW || t->currentState == STATE_TERMINATE) {
            t->cpustate_ptr = (CPUState*)(t->stack + sizeof(t->stack) - sizeof(CPUState));
            for (int i = 0; i < 30; i++) t->cpustate_ptr->x[i] = 0;
            t->cpustate_ptr->x[0] = (uint64_t)t->arg;
            t->cpustate_ptr->lr = (uint64_t)&os_thread_exit;
            t->cpustate_ptr->pc = (uint64_t)t->entrypoint;
            t->cpustate_ptr->cpsr = 0x009; 
            t->cpustate_ptr->sp = (uint64_t)t->cpustate_ptr;
            
            t->lastStartTick = current_ticks;
            t->executionsDone++;
        }
        t->currentState = STATE_READY;
    }
}

void os_join_thread(int thread_id) {
    if (thread_id < 0 || thread_id >= numThreads) return;
    if (thread_id == currentThread_idx) return; 
    if (!isSchedulingEnabled) {
        print_dbg("\n[MULTASKING-Driver] os_join_thread called inside atomic block! Deadlock avoided.\n");
        return;
    }
    while (threads[thread_id].currentState != STATE_TERMINATE && threads[thread_id].currentState != STATE_NEW) {
        thread_sleep(1);
    }
}

int os_create_thread(const char* name, void (*entrypoint)(void*), void* arg) {
    if (numThreads >= MAX_THREADS) return -1;

    Thread* t = &threads[numThreads];
    strcpy(t->name, name);
    t->entrypoint = entrypoint;
    t->arg = arg;
    t->currentState = STATE_NEW;
    t->priority = 128;
    t->deadlineOffset_ms = -1;
    t->period_ms = 0;
    t->executionsTarget = 0;
    t->executionsDone = 0;
    t->lastStartTick = 0;
    t->lastTurnaroundTime_ms = 0;
    t->wakeupTick = 0;

    os_thread_start(numThreads);
    t->executionsDone = 0; 
    t->currentState = STATE_NEW;

    numThreads++;
    return numThreads - 1; 
}

CPUState* os_schedule(CPUState* current_cpustate_ptr) {
    if (numThreads == 0 || !isSchedulingEnabled) return current_cpustate_ptr;

    if (currentThread_idx >= 0) {
        threads[currentThread_idx].cpustate_ptr = current_cpustate_ptr;
        if (threads[currentThread_idx].currentState == STATE_RUN) {
            threads[currentThread_idx].currentState = STATE_READY;
        }
    }

    uint64_t current_ticks;
    current_ticks = sysctrGetTicks();
    
    // The Wakeup & Revival Loop
    for (int i = 0; i < numThreads; i++) {
        // Wake up sleeping threads (Without wiping their stack)
        if (threads[i].currentState == STATE_WAIT_BLOCK) {
            if (current_ticks >= threads[i].wakeupTick) {
                threads[i].currentState = STATE_READY;
            }
        }
        // Revive dead periodic tasks (Wipes their stack to start fresh)
        else if ((threads[i].currentState == STATE_TERMINATE || threads[i].currentState == STATE_NEW) && i != currentThread_idx) {
            if (threads[i].executionsTarget == -1 || threads[i].executionsDone < threads[i].executionsTarget) {
                if (current_ticks >= threads[i].nextPeriodTick) {
                    os_thread_start(i); 
                }
            }
        }
    }

    if (currentSchedAlgo == SCHED_RR) {
        int startingThread_idx = currentThread_idx;
        do {
            currentThread_idx++;
            if (currentThread_idx >= numThreads) currentThread_idx = 0;
            if (threads[currentThread_idx].currentState == STATE_READY) {
                threads[currentThread_idx].currentState = STATE_RUN;
                return threads[currentThread_idx].cpustate_ptr;
            }
        } while (currentThread_idx != startingThread_idx);
    } 
    else if (currentSchedAlgo == SCHED_PRIORITY) {
        int best_thread = -1;
        int highest_pri = 999999; 
        for (int i = 0; i < numThreads; i++) {
            if (threads[i].currentState == STATE_READY && threads[i].priority < highest_pri) {
                highest_pri = threads[i].priority;
                best_thread = i;
            }
        }
        if (best_thread != -1) {
            currentThread_idx = best_thread;
            threads[currentThread_idx].currentState = STATE_RUN;
            return threads[currentThread_idx].cpustate_ptr;
        }
    }
    else if (currentSchedAlgo == SCHED_EDF) {
        int best_thread = -1;
        uint64_t earliest_deadline = 0xFFFFFFFFFFFFFFFFULL;
        for (int i = 0; i < numThreads; i++) {
            if (threads[i].currentState == STATE_READY) {
                if (best_thread == -1 || threads[i].absoluteDeadlineTick < earliest_deadline) {
                    earliest_deadline = threads[i].absoluteDeadlineTick;
                    best_thread = i;
                }
            }
        }
        if (best_thread != -1) {
            currentThread_idx = best_thread;
            threads[currentThread_idx].currentState = STATE_RUN;
            return threads[currentThread_idx].cpustate_ptr;
        }
    }

    return current_cpustate_ptr; 
}
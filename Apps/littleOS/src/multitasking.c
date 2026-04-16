#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/string.h"
#include "include/stdio.h"

uint64_t quantum_ticks;

Thread threads[MAX_THREADS];
int numThreads = 0;
int currentThread_idx = -1;
bool isSchedulingEnabled = false;
enum SchedAlgo currentSchedAlgo = SCHED_RR; 

// Tracks how long the current thread has been running in its RR slice
static uint64_t rr_slice_start_tick = 0;

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
    if (currentThread_idx >= 0 && threads[currentThread_idx].currentState == STATE_RUN) {
        threads[currentThread_idx].currentState = STATE_READY;
    }
    
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
    quantum_ticks = ((uint64_t)sysctrGetFreq() * RR_TIME_QUANTUM_MS) / 1000ULL;
    
    numThreads = 0;
    currentThread_idx = -1;
    isSchedulingEnabled = false;
    currentSchedAlgo = SCHED_RR;
    rr_slice_start_tick = 0;
    
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
        threads[i].is_silent = false;
        strcpy(threads[i].name, "EMPTY");
    }
}

void os_set_scheduling_algo(enum SchedAlgo algo) { currentSchedAlgo = algo; }
void os_stop_scheduling(void) { isSchedulingEnabled = false; }
void os_start_scheduling(void) { isSchedulingEnabled = true; }

void os_set_thread_arg(int thread_id, void* arg) {
    if (thread_id >= 0 && thread_id < numThreads) {
        threads[thread_id].arg = arg;
    }
}

int os_create_thread(const char* name, void (*entrypoint)(void*), void* arg) {
    if (numThreads >= MAX_THREADS) return -1;

    Thread* t = &threads[numThreads];
    strcpy(t->name, name);
    t->entrypoint = entrypoint;
    t->arg = arg;
    
    // Default thread parameters
    t->priority = 128;
    t->deadlineOffset_ms = -1;
    t->period_ms = 0;
    t->executionsTarget = 0;
    t->executionsDone = 0;
    t->lastStartTick = 0;
    t->lastTurnaroundTime_ms = 0;
    t->wakeupTick = 0;
    t->is_silent = false;
    t->currentState = STATE_NEW; 

    numThreads++;
    return numThreads - 1; 
}

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

bool is_current_thread_silent(void) {
    if (currentThread_idx >= 0 && currentThread_idx < MAX_THREADS) {
        return threads[currentThread_idx].is_silent;
    }
    return false;
}

void os_set_thread_silent(int thread_id, bool silent) {
    if (thread_id >= 0 && thread_id < MAX_THREADS) {
        threads[thread_id].is_silent = silent;
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

CPUState* os_schedule(CPUState* current_cpustate_ptr) {
    if (numThreads == 0 || !isSchedulingEnabled) return current_cpustate_ptr;

    // Save dying/yielding thread state without blindly demoting it
    if (currentThread_idx >= 0) {
        threads[currentThread_idx].cpustate_ptr = current_cpustate_ptr;
    }

    uint64_t current_ticks = sysctrGetTicks();
    uint32_t freq = sysctrGetFreq();
    
    // The Wakeup & Revival Loop
    for (int i = 0; i < numThreads; i++) {
        // Wake up sleeping threads
        if (threads[i].currentState == STATE_WAIT_BLOCK) {
            if (current_ticks >= threads[i].wakeupTick) {
                threads[i].currentState = STATE_READY;
            }
        }
        // Revive dead periodic tasks
        else if ((threads[i].currentState == STATE_TERMINATE || threads[i].currentState == STATE_NEW) && i != currentThread_idx) {
            if (threads[i].executionsTarget == -1 || threads[i].executionsDone < threads[i].executionsTarget) {
                if (current_ticks >= threads[i].nextPeriodTick) {
                    os_thread_start(i); 
                }
            }
        }
    }

    if (currentSchedAlgo == SCHED_RR) {
        bool needs_switch = false;
        
        // Switch if thread yielded/died
        if (currentThread_idx < 0 || threads[currentThread_idx].currentState != STATE_RUN) {
            needs_switch = true;
        } else {
            // Switch if 20ms RR quantum expired
            uint64_t elapsed_ticks = current_ticks - rr_slice_start_tick;
            if (elapsed_ticks >= quantum_ticks) {
                needs_switch = true;
                threads[currentThread_idx].currentState = STATE_READY; // Time slice over
            }
        }

        if (needs_switch) {
            int startingThread_idx = currentThread_idx < 0 ? (numThreads - 1) : currentThread_idx;
            int next_idx = startingThread_idx;
            do {
                next_idx++;
                if (next_idx >= numThreads) next_idx = 0;
                if (threads[next_idx].currentState == STATE_READY) {
                    currentThread_idx = next_idx;
                    threads[currentThread_idx].currentState = STATE_RUN;
                    rr_slice_start_tick = current_ticks; // Reset quantum clock
                    return threads[currentThread_idx].cpustate_ptr;
                }
            } while (next_idx != startingThread_idx);
            
            // If no other thread is READY, let the current thread keep running
            if (currentThread_idx >= 0 && threads[currentThread_idx].currentState == STATE_READY) {
                threads[currentThread_idx].currentState = STATE_RUN;
                rr_slice_start_tick = current_ticks;
                return threads[currentThread_idx].cpustate_ptr;
            }
        }
    } 
    else if (currentSchedAlgo == SCHED_PRIORITY) {
        int best_thread = -1;
        int highest_pri = 999999; 
        
        // Scan both RUN and READY threads to find the absolute highest priority
        for (int i = 0; i < numThreads; i++) {
            if ((threads[i].currentState == STATE_READY || threads[i].currentState == STATE_RUN) && threads[i].priority < highest_pri) {
                highest_pri = threads[i].priority;
                best_thread = i;
            }
        }
        
        if (best_thread != -1) {
            if (best_thread != currentThread_idx) {
                // Safely preempt the current thread if it was running
                if (currentThread_idx >= 0 && threads[currentThread_idx].currentState == STATE_RUN) {
                    threads[currentThread_idx].currentState = STATE_READY;
                }
                currentThread_idx = best_thread;
                threads[currentThread_idx].currentState = STATE_RUN;
            }
            return threads[currentThread_idx].cpustate_ptr;
        }
    }
    else if (currentSchedAlgo == SCHED_EDF) {
        int best_thread = -1;
        uint64_t earliest_deadline = 0xFFFFFFFFFFFFFFFFULL;
        
        for (int i = 0; i < numThreads; i++) {
            if (threads[i].currentState == STATE_READY || threads[i].currentState == STATE_RUN) {
                if (best_thread == -1 || threads[i].absoluteDeadlineTick < earliest_deadline) {
                    earliest_deadline = threads[i].absoluteDeadlineTick;
                    best_thread = i;
                }
            }
        }
        
        if (best_thread != -1) {
            if (best_thread != currentThread_idx) {
                // Safely preempt the current thread if it was running
                if (currentThread_idx >= 0 && threads[currentThread_idx].currentState == STATE_RUN) {
                    threads[currentThread_idx].currentState = STATE_READY;
                }
                currentThread_idx = best_thread;
                threads[currentThread_idx].currentState = STATE_RUN;
            }
            return threads[currentThread_idx].cpustate_ptr;
        }
    }

    return current_cpustate_ptr; 
}
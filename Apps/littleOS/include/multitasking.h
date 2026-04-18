#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>
#include "include/common_macros.h"

/**
 * @brief process states based on the process state diagram
 */
enum ThreadState {
    STATE_NEW,
    STATE_READY,
    STATE_RUN,
    STATE_WAIT_BLOCK,
    STATE_TERMINATE,
    STATE_SUSPEND_READY,
    STATE_SUSPEND_WAIT
};

/**
 * @brief available scheduling algorithms for the rtos
 */
enum SchedAlgo {
    SCHED_RR,       // round robin (default time slicing) 
    SCHED_PRIORITY, // fixed priority preemptive scheduling 
    SCHED_EDF       // earliest deadline first dynamic scheduling 
};

// Represents exactly 800 bytes pushed onto the stack by STORE_CONTEXT
typedef struct {
    uint64_t x[30]; // 240 bytes: General purpose registers 0-29
    uint64_t lr;    // 8 bytes: Link register (x30)
    uint64_t pc;    // 8 bytes: Program counter (elr_el2)
    uint64_t cpsr;  // 8 bytes: Current program status (spsr_el2)
    uint64_t sp;    // 8 bytes: Original stack pointer
    uint64_t fpsr;  // 8 bytes: Floating-Point Status Register
    uint64_t fpcr;  // 8 bytes: Floating-Point Control Register
    uint64_t q[64]; // 512 bytes: 32 x 128-bit NEON/FPU registers (q0-q31)
} CPUState;

typedef struct {
    __attribute__((aligned(16))) uint8_t stack[THREAD_STACK_SIZE]; // physical stack memory for thread 
    CPUState* cpustate_ptr; // fake cpu state stored at top of stack 
    void (*entrypoint)(void*); // function pointer for thread logic 
    void* arg; // arguments for function 
    
    char name[16]; // human readable name for stat command 
    
    // state tracking
    enum ThreadState currentState; // maps to process state diagram 

    // rtos parameters
    int priority; // 0 is highest priority, 255 is lowest 
    int deadlineOffset_ms; // relative time to finish job in milliseconds (-1 for infinite) 
    uint64_t absoluteDeadlineTick; // exact hardware tick when job must be done 
    
    // periodic parameters
    uint32_t period_ms; // if > 0, task will auto revive after this many ms 
    uint64_t nextPeriodTick; // hardware tick when dead task should wake up 
    
    // execution tracking
    int executionsTarget; // how many times to run (-1 for infinite) 
    int executionsDone;   // how many times it has finished reviving 
    
    // profiling
    uint64_t lastStartTick; // the hardware tick when the thread was last dispatched 
    uint32_t lastTurnaroundTime_ms; // turnaround time of the last completed execution 
    
    // sleep tracking
    uint64_t wakeupTick; // exact hardware tick when sleeping thread should resume 

    // extra
    bool is_silent; // Mute button for print_dbg

} Thread;

extern Thread threads[MAX_THREADS];
extern int currentThread_idx; // index of currently running thread in threads array, -1 if no thread is running
extern int numThreads; // total number of threads created so far (including dead ones)
extern bool isSchedulingEnabled; // global flag to control preemptive switching 
extern enum SchedAlgo currentSchedAlgo; // current active scheduling algorithm 

/**
 * @brief zeros out the thread array and resets scheduler state
 */
void scheduler_init(void);

/**
 * @brief resumes normal time slicing
 */
void scheduling_start(void);

/**
 * @brief pauses the scheduler so current thread keeps cpu exclusively
 */
void scheduling_stop(void);

/**
 * @brief changes the active scheduling algorithm on the fly
 * @param algo the algorithm to switch to (SCHED_RR, SCHED_PRIORITY, SCHED_EDF)
 */
void scheduling_set_algo(enum SchedAlgo algo);

/**
 * @brief called by hardware timer to swap registers based on active algorithm
 * @param current_state registers of dying thread
 * @return registers of new thread
 */
CPUState* schedule(CPUState* current_state);


/**
 * @brief creates dormant thread with a name in memory
 * @param name human readable name for the task
 * @param entrypoint c function for thread
 * @param arg params
 * @return assigned thread id
 */
int thread_create(const char* name, void (*entrypoint)(void*), void* arg); 

/**
 * @brief Sets the priority for a specific thread
 * @param thread_id id to modify
 * @param priority 0 is highest, lower number means higher priority
 */
void thread_set_priority(int thread_id, int priority);

/**
 * @brief Sets the execution deadline for a specific thread
 * @param thread_id id to modify
 * @param deadline_ms milliseconds it has to complete once started
 */
void thread_set_deadline(int thread_id, int deadline_ms);

/**
 * @brief Sets the periodicity for a specific thread
 * @param thread_id id to modify
 * @param period_ms how often it repeats (0 for one-shot)
 */
void thread_set_period(int thread_id, uint32_t period_ms);

/**
 * @brief Sets the execution target for a specific thread
 * @param thread_id id to modify
 * @param exec_target number of times to run (-1 for infinite)
 */
void thread_set_exec_target(int thread_id, int exec_target);

/**
 * @brief Updates the argument pointer for a specific thread.
 * @param thread_id id to modify
 * @param arg new argument pointer to pass when thread starts
 */
void thread_set_arg(int thread_id, void* arg);

/**
 * @brief sets the is_silent status for a specific thread
 * @param thread_id id of the thread to modify
 * @param is_silent true to mute the thread, false to unmute
 */
void thread_set_is_silent(int thread_id, bool is_silent);

/**
 * @brief if NEW or TERMINATED, wakes up a thread, calculating its absolute deadline and resetting stack
 * @param thread_id id to wake up
 */
void thread_start(int thread_id);

/**
 * @brief blocks the current thread for a specific time, yielding the cpu to other threads
 * @param ms time to sleep in milliseconds
 */
void thread_sleep(uint32_t ms);

/**
 * @brief gives up remaining time slice immediately
 */
void os_yield(void);

/**
 * @brief blocks the calling thread until the target thread finishes execution
 * @param thread_id id of the thread to wait for
 */
void thread_join(int thread_id);

/**
 * @brief puts thread to sleep
 * @param thread_id id to pause
 */
void thread_suspend(int thread_id);

/**
 * @brief forcefully and immediately terminates a thread mid-execution
 * @param thread_id id of the thread to kill
 */
void thread_kill(int thread_id);

/**
 * @brief kills thread gracefully when its function returns
 */
void thread_exit(void);

/**
 * @brief checks if the current thread is currently muted for print_dbg
 * @return true if thread is is_silent, false otherwise
 */
bool is_current_thread_silent(void);

#endif
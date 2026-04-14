#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 16 // total max threads allowed in system

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

typedef struct {
    uint64_t x[30]; // general purpose registers 
    uint64_t lr;    // link register 
    uint64_t pc;    // program counter 
    uint64_t cpsr;  // current program status 
    uint64_t sp;    // stack pointer 
} CPUState;

typedef struct {
    __attribute__((aligned(16))) uint8_t stack[4096]; // physical stack memory for thread 
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
extern int currentThread_idx;
extern int numThreads;
extern bool isSchedulingEnabled; // global flag to control preemptive switching 
extern enum SchedAlgo currentSchedAlgo; // current active scheduling algorithm 

/**
 * @brief Initializes the hardware timer to generate periodic interrupts.
 * @param period_ms The interrupt frequency in milliseconds.
 */
void os_timer_init(int period_ms);

/**
 * @brief zeros out the thread array and resets scheduler state
 */
void os_init_scheduler(void);

/**
 * @brief creates dormant thread with a name in memory
 * @param name human readable name for the task
 * @param entrypoint c function for thread
 * @param arg params
 * @return assigned thread id
 */
int os_create_thread(const char* name, void (*entrypoint)(void*), void* arg); 

/**
 * @brief configures rtos scheduling parameters for a task
 * @param thread_id id to modify
 * @param priority 0 is highest, lower number means higher priority
 * @param deadline_ms milliseconds it has to complete once started
 * @param period_ms how often it repeats (0 for one-shot)
 * @param exec_target number of times to run (-1 for infinite)
 */
void os_set_thread_rtos(int thread_id, int priority, int deadline_ms, uint32_t period_ms, int exec_target);

/**
 * @brief Updates the argument pointer for a specific thread.
 */
void os_set_thread_arg(int thread_id, void* arg);

/**
 * @brief wakes up a thread, calculating its absolute deadline and resetting stack if dead
 * @param thread_id id to wake up
 */
void os_thread_start(int thread_id);

/**
 * @brief blocks the calling thread until the target thread finishes execution
 * @param thread_id id of the thread to wait for
 */
void os_join_thread(int thread_id);

/**
 * @brief puts thread to sleep
 * @param thread_id id to pause
 */
void os_suspend_thread(int thread_id);

/**
 * @brief forcefully and immediately terminates a thread mid-execution
 * @param thread_id id of the thread to kill
 */
void os_kill_thread(int thread_id);

/**
 * @brief blocks the current thread for a specific time, yielding the cpu to other threads
 * @param ms time to sleep in milliseconds
 */
void thread_sleep(uint32_t ms);

/**
 * @brief checks if the current thread is currently muted for print_dbg
 * @return true if thread is silent, false otherwise
 */
bool is_current_thread_silent(void);

/**
 * @brief sets the silent status for a specific thread
 * @param thread_id id of the thread to modify
 * @param silent true to mute the thread, false to unmute
 */
void os_set_thread_silent(int thread_id, bool silent);

/**
 * @brief gives up remaining time slice immediately
 */
void os_yield(void);

/**
 * @brief pauses the scheduler so current thread keeps cpu exclusively
 */
void os_stop_scheduling(void);

/**
 * @brief resumes normal time slicing
 */
void os_start_scheduling(void);

/**
 * @brief changes the active scheduling algorithm on the fly
 * @param algo the algorithm to switch to (SCHED_RR, SCHED_PRIORITY, SCHED_EDF)
 */
void os_set_scheduling_algo(enum SchedAlgo algo);

/**
 * @brief called by hardware timer to swap registers based on active algorithm
 * @param current_state registers of dying thread
 * @return registers of new thread
 */
CPUState* os_schedule(CPUState* current_state);

/**
 * @brief kills thread gracefully when its function returns
 */
void os_thread_exit(void);

/**
 * @brief returns a string representation of the thread state
 * @param state the ThreadState enum value
 * @return human readable string of the state
 */
const char* get_thread_state_name(enum ThreadState state);

#endif
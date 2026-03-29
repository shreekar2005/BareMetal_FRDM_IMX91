#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 16 /**< total max threads allowed in system */

/**
 * @brief available scheduling algorithms for the rtos
 */
enum SchedAlgo {
    SCHED_RR,       /**< round robin (default time slicing) */
    SCHED_PRIORITY, /**< fixed priority preemptive scheduling */
    SCHED_EDF       /**< earliest deadline first dynamic scheduling */
};

typedef struct {
    uint64_t x[30]; /**< general purpose registers */
    uint64_t lr;    /**< link register */
    uint64_t pc;    /**< program counter */
    uint64_t cpsr;  /**< current program status */
    uint64_t sp;    /**< stack pointer */
} CPUState;

typedef struct {
    __attribute__((aligned(16))) uint8_t stack[4096]; /**< physical stack memory for thread */
    CPUState* cpustate_ptr; /**< fake cpu state stored at top of stack */
    void (*entrypoint)(void*); /**< function pointer for thread logic */
    void* arg; /**< arguments for function */
    volatile bool active; /**< if false, scheduler will skip it */
    
    char name[16]; /**< human readable name for stat command */
    
    // rtos parameters
    int priority; /**< 0 is highest priority, 255 is lowest */
    int deadline_offset_ms; /**< relative time to finish job in milliseconds (-1 for infinite) */
    uint64_t absolute_deadline_tick; /**< exact hardware tick when job must be done */
    
    // periodic parameters
    uint32_t period_ms; /**< if > 0, task will auto revive after this many ms */
    uint64_t next_period_tick; /**< hardware tick when dead task should wake up */
    
    // execution tracking
    int executions_target; /**< how many times to run (-1 for infinite) */
    int executions_done;   /**< how many times it has finished reviving */
    
    // profiling
    uint64_t last_start_tick; /**< the hardware tick when the thread was last dispatched */
    uint32_t last_exec_time_ms; /**< turnaround time of the last completed execution */
    
    // sleep tracking
    volatile bool sleeping; /**< true if thread is voluntarily blocked */
    uint64_t wakeup_tick; /**< exact hardware tick when sleeping thread should resume */
} Thread;

extern Thread threads[MAX_THREADS];
extern int current_thread;
extern int num_threads;
extern bool scheduling_enabled; /**< global flag to control preemptive switching */
extern enum SchedAlgo current_algo; /**< current active scheduling algorithm */

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
 * @brief wakes up a thread, calculating its absolute deadline and resetting stack if dead
 * @param thread_id id to wake up
 */
void os_thread_start(int thread_id);

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
void os_sleep_ms(uint32_t ms);

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
CPUState* schedule_tick(CPUState* current_state);

/**
 * @brief kills thread gracefully when its function returns
 */
void os_thread_exit(void);

#endif
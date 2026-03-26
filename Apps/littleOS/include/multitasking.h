#ifndef MULTITASKING_H
#define MULTITASKING_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 16 /**< total max threads allowed in system */

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
    bool active; /**< if false, scheduler will skip it */
} Thread;

extern Thread threads[MAX_THREADS];
extern int current_thread;
extern int num_threads;
extern bool scheduling_enabled; /**< global flag to control preemptive switching */

/**
 * @brief zeros out the thread array
 */
void os_init_scheduler(void);

/**
 * @brief creates dormant thread in memory
 * @param entrypoint c function for thread
 * @param arg params
 * @return assigned thread id
 */
int os_create_thread(void (*entrypoint)(void*), void* arg); 

/**
 * @brief wakes up a thread, resetting its stack if it previously exited
 * @param thread_id id to wake up
 */
void os_thread_start(int thread_id);

/**
 * @brief puts thread to sleep
 * @param thread_id id to pause
 */
void os_suspend_thread(int thread_id);

/**
 * @brief gives up remaining time slice immediately
 */
void os_yield(void);

/**
 * @brief pauses the scheduler so current thread keeps cpu
 */
void os_stop_scheduling(void);

/**
 * @brief resumes normal time slicing
 */
void os_start_scheduling(void);

/**
 * @brief called by hardware timer to swap registers
 * @param current_state registers of dying thread
 * @return registers of new thread
 */
CPUState* schedule_tick(CPUState* current_state);

/**
 * @brief kills thread gracefully
 */
void os_thread_exit(void);

#endif
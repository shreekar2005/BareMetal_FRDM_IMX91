#ifndef KMULTITASKING_H
#define KMULTITASKING_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 16

typedef struct {
    uint64_t x[30]; 
    uint64_t lr;    
    uint64_t pc;    
    uint64_t cpsr;  
    uint64_t sp;    
} CPUState;

typedef struct {
    __attribute__((aligned(16))) uint8_t stack[4096]; 
    CPUState* cpustate_ptr; 
    void (*entrypoint)(void*);
    void* arg;
    bool active;
} Thread;

extern Thread threads[MAX_THREADS];
extern int current_thread;
extern int num_threads;

void os_init_scheduler(void);
int os_create_thread(void (*entrypoint)(void*), void* arg); 

// --- POSIX-Style Thread Control ---
void os_thread_start(int thread_id);
void os_suspend_thread(int thread_id);
void os_yield(void);

CPUState* schedule_tick(CPUState* current_state);
void os_thread_exit(void);

#endif
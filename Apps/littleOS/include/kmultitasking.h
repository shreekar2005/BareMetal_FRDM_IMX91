#ifndef _KMULTITASKING_H
#define _KMULTITASKING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Represents the state of the ARM64 CPU registers.
typedef struct {
    uint64_t x[30]; // General purpose registers X0-X29
    uint64_t lr;    // Link Register X30 
    uint64_t pc;    // Program Counter 
    uint64_t cpsr;  // Current Program Status Register 
    uint64_t sp;    // Stack Pointer 
} __attribute__((packed)) CPUState;

typedef struct {
    uint8_t stack[4096] __attribute__((aligned(16))); // 4KB Private Stack
    CPUState* cpustate;
    void (*entrypoint)(void*);
    void* arg;
    bool active;
} Thread;

#define MAX_THREADS 16

// OS API
void os_init_scheduler(void);
bool os_create_thread(void (*entrypoint)(void*), void* arg);
void os_start(void);

// The Core Context Switchers
CPUState* schedule_tick(CPUState* current_state);
extern void os_yield(void); 

#endif
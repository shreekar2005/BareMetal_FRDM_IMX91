#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "LPUART.h"
#include "SYS_CTR.h"
#include "kmultitasking.h"
#include "cli.h" // Bring in the CLI module

#define LED_PIN (1 << 4)

// ==========================================
// GLOBAL STATE (Shared with cli.c)
// ==========================================
volatile bool run_led = false;
volatile bool os_halt = false;

volatile char print_buffer[128];
volatile int print_count = 0;
volatile bool print_active = false;

// ==========================================
// KERNEL UTILITIES
// ==========================================
void yield_delay_ms(uint32_t ms) {
    uint64_t start_ticks = sys_ctr_get_ticks();
    uint64_t wait_ticks = ms * (sys_ctr_get_freq() / 1000);
    
    while ((sys_ctr_get_ticks() - start_ticks) < wait_ticks) {
        os_yield(); 
    }
}

// ==========================================
// BACKGROUND WORKER THREADS
// ==========================================
void led_thread(void* arg) {
    while(!os_halt) {
        if(run_led) {
            GPIO2->PSOR = LED_PIN; 
            yield_delay_ms(100);        
            GPIO2->PCOR = LED_PIN;
            yield_delay_ms(100); 
        } else {
            os_yield();
        }
    }
}

void print_thread(void* arg) {
    while(!os_halt) {
        if (print_active && print_count > 0) {
            uart_print_string(LPUART1, "\r\n[Worker] ");
            uart_print_string(LPUART1, (const char*)print_buffer);
            print_count--;
            
            if (print_count == 0) {
                print_active = false;
                uart_print_string(LPUART1, "\r\n[System] Print job finished.\r\n> ");
            } else {
                yield_delay_ms(500); 
            }
        } else {
            os_yield(); 
        }
    }
}

// ==========================================
// OS BOOTLOADER
// ==========================================
int main() {
    GPIO2->PDDR |= LED_PIN; 
    
    uart_print_string(LPUART1, "\033[2J\033[H"); 
    uart_print_string(LPUART1, "=================================\r\n");
    uart_print_string(LPUART1, "     littleOS Tickless Core      \r\n");
    uart_print_string(LPUART1, "=================================\r\n");

    os_init_scheduler();

    // Spawn the threads
    os_create_thread(led_thread, NULL);
    os_create_thread(print_thread, NULL);
    os_create_thread(input_thread, NULL); // Sourced from cli.c!

    os_start(); // Hand control to the scheduler

    // If the 'shutdown' command is issued, threads exit and we land here.
    uart_print_string(LPUART1, "\r\n[Kernel] System safely halted. You may power off.\r\n");
    while(1) {
        __asm__ volatile("wfi"); 
    }
    
    return 0;
}
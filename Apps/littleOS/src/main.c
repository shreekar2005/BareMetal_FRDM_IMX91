#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "LPUART.h"
#include "SYS_CTR.h"
#include "kmultitasking.h"
#include "cli.h" 

#define LED_PIN (1 << 4)

volatile bool run_led = false;
volatile bool os_halt = false;
volatile char print_buffer[128];
volatile int print_count = 0;
volatile bool print_active = false;

extern void os_timer_init(void);

// ==========================================
// THE HEXADECIMAL CRASH DECODER
// ==========================================
void print_hex(uint64_t val) {
    uart_print_string(LPUART1, "0x");
    for (int i = 60; i >= 0; i -= 4) {
        int hex = (val >> i) & 0xF;
        uart_putchar(LPUART1, hex < 10 ? '0' + hex : 'A' + (hex - 10));
    }
}

// Called directly by vector.S when the CPU traps!
void os_fatal_error(uint64_t esr, uint64_t elr, uint64_t far) {
    uart_print_string(LPUART1, "\r\n\r\n=================================\r\n");
    uart_print_string(LPUART1, "!!! FATAL CPU EXCEPTION !!!\r\n");
    
    uart_print_string(LPUART1, "ESR_EL2 (Reason)  : "); print_hex(esr); uart_print_string(LPUART1, "\r\n");
    uart_print_string(LPUART1, "ELR_EL2 (Address) : "); print_hex(elr); uart_print_string(LPUART1, "\r\n");
    uart_print_string(LPUART1, "FAR_EL2 (Memory)  : "); print_hex(far); uart_print_string(LPUART1, "\r\n");
    
    uart_print_string(LPUART1, "System Halted.\r\n=================================\r\n");
    while(1) { __asm__ volatile("wfi"); } 
}

// ==========================================

void led_thread(void* arg) {
    uart_print_string(LPUART1, "\r\n[Thread 1] LED Controller Started!\r\n");
    while(!os_halt) {
        if(run_led) {
            GPIO2->PSOR = LED_PIN; 
            sys_ctr_delay_ms(100);        
            GPIO2->PCOR = LED_PIN;
            sys_ctr_delay_ms(100); 
        }
    }
}

void print_thread(void* arg) {
    uart_print_string(LPUART1, "[Thread 2] Print Worker Started!\r\n");
    while(!os_halt) {
        if (print_active && print_count > 0) {
            uart_print_string(LPUART1, "\r\n[Worker] ");
            uart_print_string(LPUART1, (const char*)print_buffer);
            print_count--;
            
            if (print_count == 0) {
                print_active = false;
                uart_print_string(LPUART1, "\r\n[System] Print job finished.\r\n> ");
            } else {
                sys_ctr_delay_ms(500); 
            }
        }
    }
}

int main() {
    GPIO2->PDDR |= LED_PIN; 
    
    uart_print_string(LPUART1, "\033[2J\033[H"); 
    uart_print_string(LPUART1, "=================================\r\n");
    uart_print_string(LPUART1, "     littleOS Preemptive Core    \r\n");
    uart_print_string(LPUART1, "=================================\r\n");

    uart_print_string(LPUART1, "[Boot] Step 1: Initializing Scheduler...\r\n");
    os_init_scheduler();

    uart_print_string(LPUART1, "[Boot] Step 2: Creating Threads...\r\n");
    os_create_thread(led_thread, NULL);
    os_create_thread(print_thread, NULL);
    os_create_thread(input_thread, NULL);

    uart_print_string(LPUART1, "[Boot] Step 3: Entering timer_init() & GIC setup...\r\n");
    os_timer_init(); 
    
    uart_print_string(LPUART1, "[Boot] Step 4: Setup complete! Calling os_start()...\r\n");
    os_start();      

    uart_print_string(LPUART1, "\r\n[Kernel] System safely halted.\r\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
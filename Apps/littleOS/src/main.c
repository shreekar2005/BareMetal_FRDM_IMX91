#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/cli.h" 
#include "include/gic.h"
#include "include/stdio.h"

#define LED_PIN (1 << 4)

volatile bool os_halt = false;
volatile char print_buffer[128];
volatile int print_count = 0;

int cli_thread_id;
int led_blink_thread_id;
int print_thread_id;
int atomic_print_thread_id;

extern void os_timer_init(void);

void os_fatal_error(uint64_t esr, uint64_t elr, uint64_t far, uint64_t type) {
    printf("\r\n\r\n=================================\r\n");
    printf("!!! FATAL CPU EXCEPTION !!!\r\n");
    
    if (type == 0) printf("Type: Synchronous Exception\r\n");
    if (type == 1) printf("Type: Unhandled IRQ Trap\r\n");
    if (type == 2) printf("Type: FIQ\r\n");
    if (type == 3) printf("Type: SError (System Bus Fault)\r\n");

    printf("ESR_EL2 (Reason)  : 0x%016llX\r\n", esr);
    printf("ELR_EL2 (Address) : 0x%016llX\r\n", elr);
    printf("FAR_EL2 (Memory)  : 0x%016llX\r\n", far);
    
    printf("System Halted.\r\n=================================\r\n");
    while(1) { __asm__ volatile("wfi"); } 
}

void led_blink_thread(void* arg) {
    // twice led blink
    GPIO2->PSOR = LED_PIN; 
    sys_ctr_delay_ms(300);        
    GPIO2->PCOR = LED_PIN;
    sys_ctr_delay_ms(300);
    GPIO2->PSOR = LED_PIN; 
    sys_ctr_delay_ms(300);        
    GPIO2->PCOR = LED_PIN;
}

void print_thread(void* arg) {
    // one-shot print loop
    for(int i = 0; i < print_count; i++) {
        printf("%s", (const char*)print_buffer);
        sys_ctr_delay_ms(10);
    }
    printf("\r\n[System] Print job finished.\r\n> ");
}

void atomic_print_thread(void* arg) {
    // block hardware timer preemptions
    os_stop_scheduling();
    
    for(int i = 0; i < print_count; i++) {
        printf("%s", (const char*)print_buffer);
        sys_ctr_delay_ms(10); 
    }
    printf("\r\n[System] Atomic print job finished.\r\n> ");
    
    // release cpu back to time slicer
    os_start_scheduling();
}

int main() {
    GPIO2->PDDR |= LED_PIN; 
    
    printf("\033[2J\033[H"); 
    printf("=================================\r\n");
    printf("     littleOS Preemptive Core    \r\n");
    printf("=================================\r\n");

    printf("[Boot] Initializing Scheduler...\r\n");
    os_init_scheduler();

    printf("[Boot] Creating Threads...\r\n");
    cli_thread_id = os_create_thread(input_thread, NULL);
    led_blink_thread_id = os_create_thread(led_blink_thread, NULL);
    print_thread_id = os_create_thread(print_thread, NULL);
    atomic_print_thread_id = os_create_thread(atomic_print_thread, NULL);
    
    os_thread_start(cli_thread_id); 

    printf("[Boot] Initializing GIC...\r\n");
    gic_init(); 

    printf("[Boot] Entering timer_init()...\r\n");
    os_timer_init(); 
    
    printf("[Boot] Setup complete! Calling os_start()...\r\n");
    os_start();      

    printf("\r\n[Kernel] System safely halted.\r\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "SYS_CTR.h"
#include "include/multitasking.h"
#include "include/cli.h" 
#include "include/gic.h"
#include "include/stdio.h"
#include "include/autotasks.h"

volatile bool os_halt = false;
volatile char print_buffer[128];
int cli_thread_id;

extern void os_timer_init(void);
extern void os_start(void);

void os_fatal_error(uint64_t esr, uint64_t elr, uint64_t far, uint64_t type) {
    printf("\r\n\r\n=================================\r\n");
    printf("!!! FATAL CPU EXCEPTION !!!\r\n");
    if (type == 0) printf("Type: Synchronous Exception\r\n");
    if (type == 1) printf("Type: Unhandled IRQ Trap\r\n");
    if (type == 2) printf("Type: FIQ\r\n");
    if (type == 3) printf("Type: SError (System Bus Fault)\r\n");

    printf("ESR_EL2 (Reason) : 0x%016llX\r\n", esr);
    printf("ELR_EL2 (Address) : 0x%016llX\r\n", elr);
    printf("FAR_EL2 (Memory) : 0x%016llX\r\n", far);
    printf("System Halted.\r\n=================================\r\n");
    while(1) { __asm__ volatile("wfi"); }
} 

void hardware_init(void) {
    setPinMode(GPIO2, 4, OUTPUT_MODE);  // LED pin : output mode ~ (GPIO2->PDDR |= (1 << 4);)
}
int main() {
    hardware_init(); 
    
    printf("\033[2J\033[H"); 
    printf("=================================\r\n");
    printf("     littleOS RTOS Core          \r\n");
    printf("=================================\r\n");

    printf("[Boot] Initializing Scheduler...\r\n");
    os_init_scheduler();

    printf("[Boot] Initializing Task Registry...\r\n");
    init_all_tasks(); 

    printf("[Boot] Starting Core CLI...\r\n");
    cli_thread_id = os_create_thread("CLI", input_thread, NULL);
    os_set_thread_rtos(cli_thread_id, 128, -1, 0, 1);
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
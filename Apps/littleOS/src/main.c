#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "SYS_CTR.h"
#include "IOMUX.h"
#include "LPUART.h"
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
    setPinMode(GPIO2, 4, OUTPUT_MODE); // P11 Pin 7 (GPIO_IO04) -> LED pin : output mode ~ (GPIO2->PDDR |= (1 << 4);)

    setPinMux(MUX_REG_GPIO_IO14, AF_MODE_LPUART4, 0); // P11 Pin 8 (GPIO_IO14) -> ESP RX : AF mode 6 (LPUART4_TX)
    setPinMux(MUX_REG_GPIO_IO15, AF_MODE_LPUART4, 0); // P11 Pin 10 (GPIO_IO15) -> ESP TX : AF mode 6 (LPUART4_RX)

    /** Configure the DAISY (Select Input) register for LPUART4 RX 
     * This is need if more than one pad can be used for input for the same peripheral (like same UART taking input on * different Physical pads. That may currupt the input, to avoid that we need to select one pad as input using the * DAISY register)
     * */
    volatile uint32_t *daisy_reg_lpuart4 = (volatile uint32_t *)DAISY_REG_LPUART4_RX;
    *daisy_reg_lpuart4 = DAISY_VALUE_IO15_LPUART4;

    // Initialize UART3 for your USB-C Debug Console
    initUART3(115200, 24000000); // already done by U-BOOT, but doing it again for good practice
    
    // Initialize UART4 for your ESP8266 Wi-Fi Bridge
    initUART4(115200, 24000000);
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
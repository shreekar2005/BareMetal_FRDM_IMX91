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
#include "include/esp8266.h"

extern void os_timer_init(void);
extern void os_start(void);

/** @brief used to print fatal errors */
void os_fatal_error(uint64_t esr, uint64_t elr, uint64_t far, uint64_t type) {
    printdbg("\r\n\r\n=================================\r\n");
    printdbg("!!! FATAL CPU EXCEPTION !!!\r\n");
    if (type == 0) printdbg("Type: Synchronous Exception\r\n");
    if (type == 1) printdbg("Type: Unhandled IRQ Trap\r\n");
    if (type == 2) printdbg("Type: FIQ\r\n");
    if (type == 3) printdbg("Type: SError (System Bus Fault)\r\n");

    printdbg("ESR_EL2 (Reason) : 0x%016llX\r\n", esr);
    printdbg("ELR_EL2 (Address) : 0x%016llX\r\n", elr);
    printdbg("FAR_EL2 (Memory) : 0x%016llX\r\n", far);
    printdbg("System Halted.\r\n=================================\r\n");
    while(1) { __asm__ volatile("wfi"); }
} 

/** @brief Initialize hardware components (keeping it universal to avoid conflicts) */
void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    // U-Boot already initialized LPUART1.
    // initLPUART1(115200, 24000000); 

    // Hardware initialization for built-in LED
    setPinMode(GPIO2, 4, OUTPUT_MODE); // P11 Pin 7 (GPIO_IO04) -> LED pin : output mode ~ (GPIO2->PDDR |= (1 << 4);)

    // Hardware initialization for ESP8266 Wi-Fi Bridge
    setPinMux(MUX_REG_GPIO_IO14, AF_MODE_LPUART4, 0); // P11 Pin 8 (GPIO_IO14) -> ESP RX : AF mode 6 (LPUART4_TX)
    setPinMux(MUX_REG_GPIO_IO15, AF_MODE_LPUART4, 0); // P11 Pin 10 (GPIO_IO15) -> ESP TX : AF mode 6 (LPUART4_RX)
    /** Configure the DAISY (Select Input) register for LPUART4 RX 
     * This is needed if more than one pads are used for input for the same peripheral pin (like same LPUART RX pin taking input from 2 physical pads(pins)). That may currupt input. To avoid that we need to select one pad at a time as an input using the DAISY register)
     * */
    volatile uint32_t *daisy_reg_lpuart4_rx = (volatile uint32_t *)DAISY_REG_LPUART4_RX;
    *daisy_reg_lpuart4_rx = DAISY_VALUE_IO15_LPUART4;
    // Initialize LPUART4 for ESP8266 Wi-Fi Bridge
    initLPUART4(115200, 24000000);

    
    
}

/** @brief Main function, entrypoint after start.S */
int main() {
    hardware_init();
    
    printdbg("\033[2J\033[H"); 
    printdbg("=================================\r\n");
    printdbg("     littleOS RTOS Core          \r\n");
    printdbg("=================================\r\n");

    // WI-FI INITIALIZATION
    init_esp_access_point("littleOS_Network", "password123");
    // init_esp_station("YourHomeWiFi", "YourPassword");
    init_esp_tcp_server(8080); // Start listening on port 8080

    printdbg("[Boot] Initializing Scheduler...\r\n");
    os_init_scheduler();

    printdbg("[Boot] Initializing Task Registry...\r\n");
    init_all_tasks(); 

    printdbg("[Boot] Starting Core CLI...\r\n");
    int cli_thread_id = os_create_thread("CLI", input_thread, NULL);
    os_set_thread_rtos(cli_thread_id, 128, -1, 0, 1);
    os_thread_start(cli_thread_id); 

    printdbg("[Boot] Initializing GIC...\r\n");
    gic_init(); 

    printdbg("[Boot] Initializing Timers...\r\n");
    os_timer_init(); 
    
    printdbg("[Boot] Setup complete! Calling os_start()...\r\n");
    os_start();      

    printdbg("\r\n[Kernel] System safely halted.\r\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
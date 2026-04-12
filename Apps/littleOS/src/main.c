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

extern void os_timer_init(void); // defined in src/timer.c
extern void os_start(void); // defined in vector.S, this starts the scheduler and never returns

/** @brief Initialize hardware components (keeping it universal to avoid conflicts) */
void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    // lpuartINIT(LPUART1, 115200, 24000000); // U-Boot already initialized LPUART1.

    // Hardware initialization for built-in LED
    gpioPinINIT(GPIO2, 4, OUTPUT_MODE); // P11 Pin 7 (GPIO_IO04) -> builtin GREEN-LED pin

    // Hardware initialization for ESP8266 Wi-Fi Bridge
    iomuxSetPadAltMode(MUX_REG_GPIO_IO14, ALT_MODE_LPUART4, 0); // P11 Pin 8 (GPIO_IO14) -> ESP RX : AF mode 6 (LPUART4_TX)
    iomuxSetPadAltMode(MUX_REG_GPIO_IO15, ALT_MODE_LPUART4, 0); // P11 Pin 10 (GPIO_IO15) -> ESP TX : AF mode 6 (LPUART4_RX)
    volatile uint32_t *daisy_reg_lpuart4_rx = (volatile uint32_t *)DAISY_REG_LPUART4_RX; // read about this
    *daisy_reg_lpuart4_rx = DAISY_VALUE_GPIO_IO15_LPUART4; // Select GPIO_IO15 (P11 Pin 10) as LPUART4 RX input
    lpuartINIT(LPUART4, 115200, 24000000); // Initialize LPUART4 for ESP8266 Wi-Fi Bridge

    // Hardware initialization for Sonar Task (HC-SR04 Radar)
    gpioPinINIT(GPIO2, 13, OUTPUT_MODE); // P11 Pin 33 (GPIO_IO13) -> builtin RED-LED pin
    gpioWrite(GPIO2, 13, LOW);
    gpioPinINIT(GPIO2, 2, OUTPUT_MODE); // P11 Pin 3 (GPIO_IO02) -> trigger pin for sonar
    gpioWrite(GPIO2, 2, LOW);
    gpioPinINIT(GPIO2, 3, INPUT_MODE); // P11 Pin 5 (GPIO_IO03) -> echo pin for sonar

}

/** @brief Main function, entrypoint after start.S */
int main() {
    hardware_init();

    print_dbg("\033[2J\033[H"); 
    print_dbg("=================================\r\n");
    print_dbg("     littleOS RTOS Core          \r\n");
    print_dbg("=================================\r\n");

    // WI-FI INITIALIZATION is done via "espinit" command in the CLI.
    // init_esp_as_access_point("littleOS_Network", "password123");
    // init_esp_as_station("shree_A52", "aspirine");
    // start_esp_tcp_server(8080); // Start listening on port 8080

    print_dbg("[Boot] Initializing Scheduler...\r\n");
    os_init_scheduler();

    print_dbg("[Boot] Initializing Task Registry...\r\n");
    init_all_tasks(); 

    // int statistics_thread_id = 5;
    // os_set_thread_rtos(statistics_thread_id, 127, -1, 5000, -1);

    print_dbg("[Boot] Initializing GIC...\r\n");
    gic_init();

    print_dbg("[Boot] Giving CPU vector_table address...\r\n");
    cpu_exceptions_init();

    print_dbg("[Boot] Registering ESP8266 IRQ...\r\n");
    esp_init();

    print_dbg("[Boot] Registering Timer IRQ...\r\n");
    os_timer_init(); 

    print_dbg("[Boot] Creating WiFi Listener Thread...\r\n");
    int wifi_listener_thread_id = os_create_thread("WiFiListener", espTCPServerListener_thread, NULL);
    os_set_thread_rtos(wifi_listener_thread_id, 128, -1, 0, 1);
    os_thread_start(wifi_listener_thread_id);

    print_dbg("[Boot] Creating CLI Thread...\r\n");
    int cli_thread_id = os_create_thread("CLI", input_thread, NULL);
    os_set_thread_rtos(cli_thread_id, 128, -1, 0, 1);
    os_thread_start(cli_thread_id); 
    
    print_dbg("[Boot] Setup complete! Starting Threads...\r\n");
    os_start();      

    print_dbg("\r\n[Kernel] System safely halted.\r\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
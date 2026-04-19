#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "GPIO.h"
#include "SYS_CTR.h"
#include "IOMUX.h"
#include "LPUART.h"
#include "GIC.h"
#include "include/timer.h"
#include "include/multitasking.h"
#include "include/shared_locks.h"
#include "include/cli.h" 
#include "include/stdio.h"
#include "include/autotasks.h"
#include "include/esp8266.h"
#include "include/datetime.h"
#include "include/common_macros.h"
#include "include/memory.h"

extern void* vector_table; // Defined in vector.S
extern void __os_start_asm(void); // defined in vector.S

/**
 * @brief Booting done... Now threads will takeover. This function should never return.
 */
void os_start(void) {
    isSchedulingEnabled = true; 
    __os_start_asm(); 
}

/** @brief Initialize hardware components (keeping it universal to avoid conflicts) */
void hardware_init(void) {
    // Hardware initialization for usb debug pin (LPUART1)
    lpuartINIT(LPUART1, 115200, 24000000); // U-Boot already initialized LPUART1, but we are doing it again.

    // Hardware initialization for built-in RGB LEDs
    gpioPinINIT(GPIO2, BUILTIN_GREEN_LED, OUTPUT_MODE); // P11 Pin 7 (GPIO_IO04) -> builtin GREEN-LED pin
    gpioPinINIT(GPIO2, BUILTIN_BLUE_LED, OUTPUT_MODE); // P11 Pin 33 (GPIO_IO12) -> builtin BLUE-LED pin
    gpioPinINIT(GPIO2, BUILTIN_RED_LED, OUTPUT_MODE); // P11 Pin 33 (GPIO_IO13) -> builtin RED-LED pin

    // Hardware initialization for ESP8266 Wi-Fi Bridge
    iomuxSetPadAltMode(MUX_REG_GPIO_IO14, ALT_MODE_LPUART4, 0); // P11 Pin 8 (GPIO_IO14) -> ESP RX : AF mode 6 (LPUART4_TX)
    iomuxSetPadAltMode(MUX_REG_GPIO_IO15, ALT_MODE_LPUART4, 0); // P11 Pin 10 (GPIO_IO15) -> ESP TX : AF mode 6 (LPUART4_RX)
    volatile uint32_t *daisy_reg_lpuart4_rx = (volatile uint32_t *)DAISY_REG_LPUART4_RX; // read about this
    *daisy_reg_lpuart4_rx = DAISY_VALUE_GPIO_IO15_LPUART4; // Select GPIO_IO15 (P11 Pin 10) as LPUART4 RX input
    lpuartINIT(LPUART4, 115200, 24000000); // Initialize LPUART4 for ESP8266 Wi-Fi Bridge

    // Hardware initialization for Sonar Task (HC-SR04 Radar)
    gpioPinINIT(GPIO2, ULTRASONIC_TRIG_PIN, OUTPUT_MODE); // P11 Pin 3 (GPIO_IO02) -> trigger pin for sonar
    gpioWrite(GPIO2, ULTRASONIC_TRIG_PIN, LOW);
    gpioPinINIT(GPIO2, ULTRASONIC_ECHO_PIN, INPUT_MODE); // P11 Pin 5 (GPIO_IO03) -> echo pin for sonar

    // Hardware initialization for Generic Interrupt Controller
    gicINIT(); // Initialize the GIC Distributor and Redistributor
    gicCPUInit((uintptr_t)&vector_table); // Giving CPU vector_table address
}

int globalVar_uninitialized;
int globalVar_initialized=1;

/** @brief Main function : Entry point for littleOS */
int main() {
    hardware_init(); // Initialize hardware components before starting threads that depend on them.
    
    print_dbg("\033[2J\033[H");
    print_dbg("[littleOS] Welcome TO littleOS RTOS :)\n");
    
    int localVar;;
    print_dbg("stack variable on                 : 0x%08X\n", &localVar);
    print_dbg("global(uninitialized) variable on : 0x%08X\n", &globalVar_uninitialized);
    print_dbg("global(initialized) variable on   : 0x%08X\n", &globalVar_initialized);
    memory_print_footprint();

    print_dbg("[Boot] Initializing Scheduler...\n");
    scheduler_init(); // Note: isSchedulingEnabled is set to false here. 

    print_dbg("[Boot] Initializing Task Registry...\n");
    init_all_tasks();

    print_dbg("[Boot] Registering ESP8266 IRQ...\n");
    esp_init();

    print_dbg("[Boot] Creating System RTC Daemon...\n");
    int rtc_thread_id = thread_create("RTC_Daemon", datetime_ticker_thread, NULL);
    thread_set_priority(rtc_thread_id, 128);
    thread_set_deadline(rtc_thread_id, -1);
    thread_set_period(rtc_thread_id, 0);
    thread_set_exec_target(rtc_thread_id, 0);
    thread_start(rtc_thread_id);

    print_dbg("[Boot] Creating WiFi Listener Thread...\n");
    int wifi_listener_thread_id = thread_create("ESPWiFiListener", espTCPServerListener_thread, NULL);
    thread_set_priority(wifi_listener_thread_id, 128);
    thread_set_deadline(wifi_listener_thread_id, -1);
    thread_set_period(wifi_listener_thread_id, 0);
    thread_set_exec_target(wifi_listener_thread_id, 0);
    thread_start(wifi_listener_thread_id);

    print_dbg("[Boot] Creating CLI Thread...\n");
    int cli_thread_id = thread_create("CLI", cli_thread, NULL);
    thread_set_priority(cli_thread_id, 128);
    thread_set_deadline(cli_thread_id, -1);
    thread_set_period(cli_thread_id, 0);
    thread_set_exec_target(cli_thread_id, 0);
    thread_start(cli_thread_id);
    
    print_dbg("[Boot] Setup complete! Starting Threads...\n");
    
    os_timer_init(1); 
    os_start();  

    print_dbg("\n[Kernel] System safely halted.\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
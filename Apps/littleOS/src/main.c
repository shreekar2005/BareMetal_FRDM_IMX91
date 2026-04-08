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

/* Simple busy-wait delay for early boot sequence */
static void boot_delay(volatile uint32_t cycles) {
    while(cycles--) {
        __asm__ volatile("nop");
    }
}

/* Reads whatever the ESP8266 sent back and prints it to the debug console */
static void print_esp_response(void) {
    char c;
    printdbg("[ESP8266] ");
    // Drain the RX buffer completely
    while ((c = lpuart_getchar_nonblocking(LPUART4)) != '\0') {
        // Echo characters to debug port
        lpuart_putchar(LPUART1, c); 
    }
    printdbg("\r\n");
}

/**
 * @brief Configures the ESP8266 to act as a TCP Server.
 * @param port The network port to listen on (e.g., 8080).
 */
void init_esp_tcp_server(int port) {
    printdbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections (Required for Server mode) */
    printesp("AT+CIPMUX=1\r\n");
    boot_delay(5000000);
    print_esp_response();

    /* Start the Server: AT+CIPSERVER=<mode>,<port> 
     * Mode 1 = Create server */
    printesp("AT+CIPSERVER=1,%d\r\n", port);
    boot_delay(5000000);
    print_esp_response();
    
    /* Check the ESP's IP address so you know where to connect */
    printesp("AT+CIFSR\r\n");
    boot_delay(5000000);
    print_esp_response();

    printdbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}

/**
 * @brief Configures the ESP8266 as an Access Point (AP).
 * You can connect your phone/laptop to this network.
 * * @param ssid The name of the Wi-Fi network to broadcast.
 * @param password The password for the network (must be >= 8 characters).
 */
void init_esp_access_point(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Access Point (AP) Mode...\r\n");

    /* Test communication */
    printesp("AT\r\n");
    boot_delay(5000000); 
    print_esp_response();

    /* Set Wi-Fi Mode to 2 (SoftAP) */
    printesp("AT+CWMODE=2\r\n");
    boot_delay(5000000);
    print_esp_response();

    /* Configure the AP: AT+CWSAP="ssid","pwd",channel,encryption 
     * Encryption 3 = WPA2_PSK */
    printesp("AT+CWSAP=\"%s\",\"%s\",1,3\r\n", ssid, password);
    boot_delay(15000000); // AP creation takes slightly longer
    print_esp_response();
    
    printdbg("[Wi-Fi] Access Point '%s' is now broadcasting.\r\n", ssid);
}

/**
 * @brief Configures the ESP8266 as a Station (STA).
 * It will connect to an existing Wi-Fi router.
 * * @param ssid The name of the router to connect to.
 * @param password The password of the router.
 */
void init_esp_station(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Station Mode...\r\n");

    /* Test communication */
    printesp("AT\r\n");
    boot_delay(5000000);
    print_esp_response();

    /* Set Wi-Fi Mode to 1 (Station) */
    printesp("AT+CWMODE=1\r\n");
    boot_delay(5000000);
    print_esp_response();

    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    printesp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    
    printdbg("[Wi-Fi] Attempting connection... (This takes a few seconds)\r\n");
    boot_delay(40000000); // Wi-Fi handshake takes several seconds
    print_esp_response();
    
    printdbg("[Wi-Fi] Station connection sequence complete.\r\n");
}


void hardware_init(void) {
    setPinMode(GPIO2, 4, OUTPUT_MODE); // P11 Pin 7 (GPIO_IO04) -> LED pin : output mode ~ (GPIO2->PDDR |= (1 << 4);)

    setPinMux(MUX_REG_GPIO_IO14, AF_MODE_LPUART4, 0); // P11 Pin 8 (GPIO_IO14) -> ESP RX : AF mode 6 (LPUART4_TX)
    setPinMux(MUX_REG_GPIO_IO15, AF_MODE_LPUART4, 0); // P11 Pin 10 (GPIO_IO15) -> ESP TX : AF mode 6 (LPUART4_RX)

    /** Configure the DAISY (Select Input) register for LPUART4 RX 
     * This is need if more than one pad can be used for input for the same peripheral (like same LPUART taking input on * different Physical pads. That may currupt the input, to avoid that we need to select one pad as input using the * DAISY register)
     * */
    volatile uint32_t *daisy_reg_lpuart4 = (volatile uint32_t *)DAISY_REG_LPUART4_RX;
    *daisy_reg_lpuart4 = DAISY_VALUE_IO15_LPUART4;

    
    // U-Boot already initialized LPUART1.
    // initLPUART1(115200, 24000000); 
    
    // Initialize LPUART4 for your ESP8266 Wi-Fi Bridge
    initLPUART4(115200, 24000000);
}

int main() {
    hardware_init(); 
    
    printdbg("\033[2J\033[H"); 
    printdbg("=================================\r\n");
    printdbg("     littleOS RTOS Core          \r\n");
    printdbg("=================================\r\n");

    /* --- WI-FI INITIALIZATION --- */
    init_esp_access_point("littleOS_Network", "password123");
    
    // Start listening on port 8080
    init_esp_tcp_server(8080);

    printdbg("[Boot] Initializing Scheduler...\r\n");
    os_init_scheduler();

    printdbg("[Boot] Initializing Task Registry...\r\n");
    init_all_tasks(); 

    printdbg("[Boot] Starting Core CLI...\r\n");
    cli_thread_id = os_create_thread("CLI", input_thread, NULL);
    os_set_thread_rtos(cli_thread_id, 128, -1, 0, 1);
    os_thread_start(cli_thread_id); 

    printdbg("[Boot] Initializing GIC...\r\n");
    gic_init(); 

    printdbg("[Boot] Entering timer_init()...\r\n");
    os_timer_init(); 
    
    printdbg("[Boot] Setup complete! Calling os_start()...\r\n");
    os_start();      

    printdbg("\r\n[Kernel] System safely halted.\r\n");
    while(1) { __asm__ volatile("wfi"); }
    
    return 0;
}
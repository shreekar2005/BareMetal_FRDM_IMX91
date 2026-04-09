#include "include/esp8266.h"
#include "include/stdio.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/multitasking.h"
#include "SYS_CTR.h"
#include <stdbool.h>

/**
 * @brief custom printrawesp for ESP Wi-Fi port
 * * this function formats a string and prints it to the ESP network module.
 * @details supports the exact same format specifiers and flags as printdbg.
 * * @param format the null-terminated format string
 * @param ... variable arguments
 * @return total number of characters printed
 */
static int printrawesp(const char *format, ...) {
    va_list args;
    va_start(args, format);
    // Print to your Wi-Fi module (LPUART4)
    int chars_written = vprint_uart(LPUART4, format, args); 
    va_end(args);
    return chars_written;
}

/* Actively listens to the ESP8266 until it sees "OK" or "ERROR", or times out */
static bool wait_for_esp_ok(uint32_t timeout_sec) {
    char c;
    char prev = 0;
    bool success = false;
    bool timed_out = true;
    
    uint64_t target_clock_tick = sys_ctr_get_ticks() + timeout_sec * sys_ctr_get_freq(); 
    printdbg("[ESP8266] ");
    
    while (sys_ctr_get_ticks() < target_clock_tick) {
        /* clear Overrun errors just in case */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        c = lpuart_getchar_nonblocking(LPUART4);
        
        if (c != '\0') {
            
            if(c != '\r' && c != '\n') printdbg("%c", c);
            if(c == '\n') printdbg("\\r");
            if(c == '\r') printdbg("\\r");

            // "OK"?
            if (prev == 'O' && c == 'K') {
                // Read the final '\r' and '\n' to clear the pipe before exiting
                uint64_t flush_target = sys_ctr_get_ticks() + (sys_ctr_get_freq() / 100); 
                while (sys_ctr_get_ticks() < flush_target) {
                    char flush_c = lpuart_getchar_nonblocking(LPUART4);
                    if (flush_c == '\n') break;
                }
                success = true;
                timed_out = false;
                break;
            }
            
            // ERR"OR"? 
            if (prev == 'O' && c == 'R') {
                success = false;
                timed_out = false;
                break;
            }

            // FA"IL"? (AT+CWJAP outputs FAIL instead of ERROR when it can't connect)
            if (prev == 'I' && c == 'L') {
                uint64_t flush_target = sys_ctr_get_ticks() + (sys_ctr_get_freq() / 100); 
                while (sys_ctr_get_ticks() < flush_target) {
                    if (lpuart_getchar_nonblocking(LPUART4) == '\n') break;
                }
                success = false;
                timed_out = false;
                break;
            }

            prev = c;
        }
        __asm__ volatile("nop"); 
    }
    
    if (timed_out) {
        printdbg("\r\n[Warning] %d sec timeout!\r\n", timeout_sec);
        return false;
    } 

    printdbg("\r\n");
    return success;
}

void init_esp_access_point(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Access Point (AP) Mode...\r\n");

    /* Test communication */
    printrawesp("AT\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to communicate with ESP8266. Check wiring and try again.\r\n");
        return;
    }

    /* Set Wi-Fi Mode to 2 (SoftAP) */
    printrawesp("AT+CWMODE=2\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to set Wi-Fi mode.\r\n");
        return;
    }

    /* Configure the AP: AT+CWSAP="ssid","pwd",channel,encryption 
     * Encryption 3 = WPA2_PSK */
    printrawesp("AT+CWSAP=\"%s\",\"%s\",6,3\r\n", ssid, password);
    if (wait_for_esp_ok(10)==false) {
        printdbg("[Wi-Fi] Failed to configure Access Point.\r\n");
        return;
    }
    
    printdbg("[Wi-Fi] Access Point '%s' is now broadcasting.\r\n", ssid);
}


void init_esp_station(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Station Mode...\r\n");

    /* Test communication */
    printrawesp("AT\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to communicate with ESP8266. Check wiring and try again.\r\n");
        return;
    }

    /* Set Wi-Fi Mode to 1 (Station) */
    printrawesp("AT+CWMODE=1\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to set Wi-Fi mode.\r\n");
        return;
    }

    printdbg("[Wi-Fi] Attempting connection... (20 sec timeout)\r\n");
    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    printrawesp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (wait_for_esp_ok(20)==false) {
        printdbg("[Wi-Fi] Failed to connect to Access Point.\r\n");
        return;
    }
    printdbg("[Wi-Fi] Station connection sequence complete.\r\n");
}

void init_esp_tcp_server(int port) {
    printdbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections (Required for Server mode) */
    printrawesp("AT+CIPMUX=1\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to enable multiple connections.\r\n");
        return;
    }

    /* Start the Server: AT+CIPSERVER=<mode>,<port> 
     * Mode 1 = Create server */
    printrawesp("AT+CIPSERVER=1,%d\r\n", port);
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to start TCP server.\r\n");
        return;
    }
    
    /* Check the ESP's IP address so you know where to connect */
    printrawesp("AT+CIFSR\r\n");
    if (wait_for_esp_ok(5)==false) {
        printdbg("[Wi-Fi] Failed to retrieve IP address.\r\n");
        return;
    }

    printdbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}


/**
 * @brief Formats a string and sends it over Wi-Fi as a TCP payload.
 */
int printesp(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    
    // format the string into our local staging buffer using the stdio helper
    int len = vprint_esp8266(buffer, format, args);
    va_end(args);
    
    if (len <= 0) return 0;

    // calculate the exact wire length.
    int wire_len = 0;
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\n') wire_len++; // account for lpuart_putchar injecting \r
        wire_len++;
    }

    // send the CIPSEND command
    printrawesp("AT+CIPSEND=0,%d\r\n", wire_len);

    // waiting for the ESP8266 to output the '>' prompt indicating it is ready
    // 2-second hardware timeout
    uint64_t target_clock_tick = sys_ctr_get_ticks() + (2 * sys_ctr_get_freq());
    while (sys_ctr_get_ticks() < target_clock_tick) {
        if (lpuart_getchar_nonblocking(LPUART4) == '>') {
            break;
        }
    }
    
    printrawesp("%s", buffer);
    return len;
}

void wifi_listener_forCLI_thread(void *arg) {
    char buffer[128];
    int idx = 0;
    
    while(1) {
        /* Clear hardware errors (Overrun) to keep the line open */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        char c = lpuart_getchar_nonblocking(LPUART4);
        
        if (c == '+') {
            
            // ENTER CRITICAL SECTION
            os_stop_scheduling();

            int local_state = 1; // 1 = wait for ':', 2 = read payload
            idx = 0;
            volatile int timeout = 1000000; // Safety timeout against infinite freezes
            
            while(timeout--) {
                char temp_c = lpuart_getchar_nonblocking(LPUART4);
                if (temp_c == '\0') continue;
                
                if (local_state == 1) {
                    if (temp_c == ':') local_state = 2; // Found payload start!
                } 
                else if (local_state == 2) {
                    // Read until newline
                    if (temp_c == '\n' || temp_c == '\r') {
                        break; // Packet fully received!
                    } else {
                        if (idx < 127) buffer[idx++] = temp_c;
                    }
                }
            }
            
            os_start_scheduling(); // Exit critical section, re-enable RTOS scheduling and timer interrupts

            /* Now process the fully captured string safely outside the critical zone */
            if (idx > 0) {
                buffer[idx] = '\0';
                printdbg("[ESP8266-remote]: %s\r\n", buffer);
                
                if (my_strncmp(buffer, "exec", 4) == 0) {
                    printdbg(">>%s", buffer);
                    handleCommand(buffer + 5); 
                }
            }
        }
    }
}
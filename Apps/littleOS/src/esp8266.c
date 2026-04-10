#include <stdbool.h>
#include "SYS_CTR.h"
#include "include/esp8266.h"
#include "include/stdio.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/multitasking.h"

/**
 * @brief custom send_to_esp for ESP Wi-Fi port
 * * this function formats a string and prints it to the ESP network module.
 * @details supports the exact same format specifiers and flags as print_dbg.
 * * @param format the null-terminated format string
 * @param ... variable arguments
 * @return total number of characters printed
 */
static int send_to_esp(const char *format, ...) {
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
    
    uint64_t targetClockTick = sysctrGetTicks() + timeout_sec * sysctrGetFreq(); 
    print_dbg("[ESP8266] ");
    
    while (sysctrGetTicks() < targetClockTick) {
        /* clear Overrun errors just in case */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        c = lpuartGetCharNonBlocking(LPUART4);
        
        if (c != '\0') {
            
            if(c != '\r' && c != '\n') print_dbg("%c", c);
            if(c == '\n') print_dbg("\\r");
            if(c == '\r') print_dbg("\\r");

            // "OK"?
            if (prev == 'O' && c == 'K') {
                // Read the final '\r' and '\n' to clear the pipe before exiting
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = lpuartGetCharNonBlocking(LPUART4);
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
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    if (lpuartGetCharNonBlocking(LPUART4) == '\n') break;
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
        print_dbg("\r\n[Warning] %d sec timeout!\r\n", timeout_sec);
        return false;
    } 

    print_dbg("\r\n");
    return success;
}

void print_esp_status(void) {
    /* Basic Hardware Ping */
    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(3) == false) {
        print_dbg("[ESP8266] Failed to communicate. Is the module powered on?\r\n");
        return;
    }

    /* Check Current Mode */
    print_dbg("\r\n[1] Operating Mode:\r\n");
    print_dbg("    (1 = Station/Client, 2 = Access Point, 3 = Both)\r\n");
    send_to_esp("AT+CWMODE?\r\n");
    if (wait_for_esp_ok(3) == false) {
        print_dbg("[ESP8266] Failed to fetch mode.\r\n");
    }

    /* Check Connection to Router (Station Mode) */
    print_dbg("\r\n[2] Connected Router (If in Station Mode):\r\n");
    send_to_esp("AT+CWJAP?\r\n");
    if (wait_for_esp_ok(5) == false) {
        print_dbg("[ESP8266] Failed to fetch router info.\r\n");
    }

    /* Check Hosted Network (Access Point Mode) */
    print_dbg("\r\n[3] Hosted Network (If in Access Point Mode):\r\n");
    send_to_esp("AT+CWSAP?\r\n");
    if (wait_for_esp_ok(3) == false) {
        print_dbg("[ESP8266] Failed to fetch AP info.\r\n");
    }

    /* IP & MAC Addresses */
    print_dbg("\r\n[4] Network Addresses:\r\n");
    print_dbg("    (STAIP = Your IP on the router, APIP = Hosted IP)\r\n");
    send_to_esp("AT+CIFSR\r\n");
    if (wait_for_esp_ok(3) == false) {
        print_dbg("[ESP8266] Failed to fetch IP addresses.\r\n");
    }
}

void init_esp_as_access_point(const char* ssid, const char* password) {
    print_dbg("\r\n[Wi-Fi] Initializing ESP8266 in Access Point (AP) Mode...\r\n");

    /* Test communication */
    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to communicate with ESP8266. Check wiring and try again.\r\n");
        return;
    }

    /* Set Wi-Fi Mode to 2 (SoftAP) */
    send_to_esp("AT+CWMODE=2\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to set Wi-Fi mode.\r\n");
        return;
    }

    /* Configure the AP: AT+CWSAP="ssid","pwd",channel,encryption 
     * Encryption 3 = WPA2_PSK */
    send_to_esp("AT+CWSAP=\"%s\",\"%s\",6,3\r\n", ssid, password);
    if (wait_for_esp_ok(10)==false) {
        print_dbg("[Wi-Fi] Failed to configure Access Point.\r\n");
        return;
    }
    
    print_dbg("[Wi-Fi] Access Point '%s' is now broadcasting.\r\n", ssid);
}


void init_esp_as_station(const char* ssid, const char* password) {
    print_dbg("\r\n[Wi-Fi] Initializing ESP8266 in Station Mode...\r\n");

    /* Test communication */
    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to communicate with ESP8266. Check wiring and try again.\r\n");
        return;
    }

    /* Set Wi-Fi Mode to 1 (Station) */
    send_to_esp("AT+CWMODE=1\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to set Wi-Fi mode.\r\n");
        return;
    }

    print_dbg("[Wi-Fi] Attempting connection... (20 sec timeout)\r\n");
    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    send_to_esp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (wait_for_esp_ok(20)==false) {
        print_dbg("[Wi-Fi] Failed to connect to Access Point.\r\n");
        return;
    }
    print_dbg("[Wi-Fi] Station connection sequence complete.\r\n");
}

void start_esp_tcp_server(int port) {
    print_dbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections (Required for Server mode) */
    send_to_esp("AT+CIPMUX=1\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to enable multiple connections.\r\n");
        return;
    }

    /* Start the Server: AT+CIPSERVER=<mode>,<port> 
     * Mode 1 = Create server */
    send_to_esp("AT+CIPSERVER=1,%d\r\n", port);
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to start TCP server.\r\n");
        return;
    }
    
    /* Check the ESP's IP address so you know where to connect */
    send_to_esp("AT+CIFSR\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to retrieve IP address.\r\n");
        return;
    }

    print_dbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}

void espTCPServerListener_thread(void *arg) {
    char buffer[128];
    int idx = 0;
    
    while(1) {
        /* Clear hardware errors (Overrun) to keep the line open */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        char c = lpuartGetCharNonBlocking(LPUART4);
        
        if (c == '+') {
            
            // ENTER CRITICAL SECTION
            os_stop_scheduling();

            int localState = 1; // 1 = wait for ':', 2 = read payload
            idx = 0;
            volatile int timeout = 1000000; // Safety timeout against infinite freezes
            
            while(timeout--) {
                char temp_c = lpuartGetCharNonBlocking(LPUART4);
                if (temp_c == '\0') continue;
                
                if (localState == 1) {
                    if (temp_c == ':') localState = 2; // Found payload start!
                } 
                else if (localState == 2) {
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
                print_dbg("[ESP8266-remote]: %s\r\n", buffer);
                
                if (strncmp(buffer, "exec ", 5) == 0) {
                    handleCommand(buffer + 5); 
                }
            }
        }
    }
}
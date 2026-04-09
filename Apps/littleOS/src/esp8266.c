#include "include/esp8266.h"
#include "include/stdio.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/multitasking.h"

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
static void wait_for_esp_ok(volatile uint32_t timeout_cycles) {
    char c;
    char prev = 0;
    
    printdbg("[ESP8266] ");
    
    while (timeout_cycles--) {
        /* clear Overrun errors just in case */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        // non-blocking read so we don't freeze forever if a wire unplugs
        c = lpuart_getchar_nonblocking(LPUART4);
        
        if (c != '\0') {
            
            if(c != '\r' && c != '\n') printdbg("%c", c);
            if(c == '\n') printdbg("\\r");
            if(c == '\r') printdbg("\\r");
            
            // "OK"?
            if (prev == 'O' && c == 'K') {
                // Read the final '\r' and '\n' to clear the pipe before exiting
                volatile int flush_timeout = 100000;
                while(flush_timeout--) {
                    if (lpuart_getchar_nonblocking(LPUART4) == '\n') break;
                }
                break;
            }
            
            // ERR"OR"? 
            if (prev == 'O' && c == 'R') {
                break;
            }
            prev = c;
        }
        __asm__ volatile("nop"); 
    }
    
    if (timeout_cycles == 0) {
        printdbg("\r\n[Warning] Hardware Timeout!\r\n");
    } else {
        printdbg("\r\n");
    }
}

void init_esp_access_point(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Access Point (AP) Mode...\r\n");

    /* Test communication */
    printrawesp("AT\r\n");
    wait_for_esp_ok(50000000); 

    /* Set Wi-Fi Mode to 2 (SoftAP) */
    printrawesp("AT+CWMODE=2\r\n");
    wait_for_esp_ok(50000000);

    /* Configure the AP: AT+CWSAP="ssid","pwd",channel,encryption 
     * Encryption 3 = WPA2_PSK */
    printrawesp("AT+CWSAP=\"%s\",\"%s\",6,3\r\n", ssid, password);
    wait_for_esp_ok(150000000); // AP creation takes slightly longer
    
    printdbg("[Wi-Fi] Access Point '%s' is now broadcasting.\r\n", ssid);
}


void init_esp_station(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Station Mode...\r\n");

    /* Test communication */
    printrawesp("AT\r\n");
    wait_for_esp_ok(50000000);

    /* Set Wi-Fi Mode to 1 (Station) */
    printrawesp("AT+CWMODE=1\r\n");
    wait_for_esp_ok(50000000);

    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    printrawesp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    
    printdbg("[Wi-Fi] Attempting connection... (This takes a few seconds)\r\n");
    wait_for_esp_ok(400000000); // Wi-Fi handshake takes several seconds
    
    printdbg("[Wi-Fi] Station connection sequence complete.\r\n");
}

void init_esp_tcp_server(int port) {
    printdbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections (Required for Server mode) */
    printrawesp("AT+CIPMUX=1\r\n");
    wait_for_esp_ok(50000000);

    /* Start the Server: AT+CIPSERVER=<mode>,<port> 
     * Mode 1 = Create server */
    printrawesp("AT+CIPSERVER=1,%d\r\n", port);
    wait_for_esp_ok(50000000);
    
    /* Check the ESP's IP address so you know where to connect */
    printrawesp("AT+CIFSR\r\n");
    wait_for_esp_ok(50000000);

    printdbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}


/* Helper to format strings into RAM so we can calculate the exact AT+CIPSEND length */
static int mini_vsprintf(char *buf, const char *fmt, va_list args) {
    char *ptr = buf;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // Skip '%'
            if (*fmt == 's') {
                char *s = va_arg(args, char *);
                if (!s) s = "(null)";
                while (*s) *ptr++ = *s++;
            } else if (*fmt == 'd') {
                int val = va_arg(args, int);
                char num_buf[16];
                int idx = 0;
                if (val < 0) { *ptr++ = '-'; val = -val; }
                if (val == 0) { *ptr++ = '0'; }
                while (val > 0) { num_buf[idx++] = (val % 10) + '0'; val /= 10; }
                while (idx > 0) *ptr++ = num_buf[--idx];
            } else if (*fmt == 'x') {
                unsigned int val = va_arg(args, unsigned int);
                char num_buf[16];
                int idx = 0;
                if (val == 0) { *ptr++ = '0'; }
                while (val > 0) { 
                    int rem = val % 16;
                    num_buf[idx++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a'); 
                    val /= 16; 
                }
                while (idx > 0) *ptr++ = num_buf[--idx];
            } else if (*fmt == 'c') {
                *ptr++ = (char)va_arg(args, int);
            } else if (*fmt == '%') {
                *ptr++ = '%';
            }
            fmt++; // Move past the format specifier
        } else {
            *ptr++ = *fmt++; // Copy normal character
        }
    }
    *ptr = '\0';
    return (int)(ptr - buf);
}

/**
 * @brief Formats a string and sends it over Wi-Fi as a TCP payload.
 */
int printesp(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    
    // format the string into our local staging buffer
    int len = mini_vsprintf(buffer, format, args);
    va_end(args);
    
    if (len <= 0) return 0;

    // calculate the exact wire length.
    int wire_len = 0;
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\n') wire_len++; // because our lpuart_print function converts \n to \r\n for the ESP, we need to account for the extra \r character in the length
        wire_len++;
    }

    // send the CIPSEND command
    printrawesp("AT+CIPSEND=0,%d\r\n", wire_len);

    // waiting for the ESP8266 to output the '>' prompt indicating it is ready
    volatile uint32_t timeout = 5000000;
    while (timeout--) {
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
                printdbg("[ESP8266]: %s\r\n", buffer);
                
                if (my_strncmp(buffer, "exec", 4) == 0) {
                    printdbg(">>%s", buffer);
                    handleCommand(buffer + 5); 
                }
            }
        }
    }
}
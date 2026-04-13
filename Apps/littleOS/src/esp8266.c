#include <stdbool.h>
#include "SYS_CTR.h"
#include "include/esp8266.h"
#include "include/stdio.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/multitasking.h"
#include "include/gic.h"
#include "include/irq.h"

// ----------------------------------- RING BUFFER ARCHITECTURE and ISR START-----------------------------------

// BUFFER ARCHITECTURE

RingBuffer esp_rx_buffer = { .head = 0, .tail = 0 };

char esp_ring_buffer_pop(void) {
    char c = '\0';
    if (esp_rx_buffer.head != esp_rx_buffer.tail) {
        c = esp_rx_buffer.data[esp_rx_buffer.tail];
        esp_rx_buffer.tail = (esp_rx_buffer.tail + 1) % ESP_RX_BUFFER_SIZE;
    }
    return c;
}

// ISR
/**
 * @brief Hardware IRQ Handler for LPUART4. 
 * This executes instantly when the ESP8266 sends a byte.
 */
CPUState* lpuart4_rx_isr(CPUState* current_state) {
    uint32_t stat = LPUART4->STAT;

    /* Clear Hardware Overrun Flags */
    if (stat & (0xF << 16)) {
        LPUART4->STAT |= (0xF << 16); 
    }

    /* Check if Receive Data Register is Full (RDRF is bit 21 in NXP LPUART) */
    if (stat & (1 << 21)) {
        char c = (char)(LPUART4->DATA & 0xFF); // Read hardware FIFO
        
        // Calculate next head position
        int next_head = (esp_rx_buffer.head + 1) % ESP_RX_BUFFER_SIZE;
        
        // If buffer isn't full, save the byte. (If full, it drops the byte)
        if (next_head != esp_rx_buffer.tail) {
            esp_rx_buffer.data[esp_rx_buffer.head] = c;
            esp_rx_buffer.head = next_head;
        }
    }

    return current_state; // Return without forcing a context switch
}

// -----------------------------------RING BUFFER ARCHITECTURE and ISR END-----------------------------------

void esp_init(void) {

    /* Register the ISR with your OS Dispatcher */
    register_irq(IMX91_LPUART4_IRQ_ID, lpuart4_rx_isr);
    
    /* Enable the Interrupt in the ARM GIC */
    gic_enable_interrupt(IMX91_LPUART4_IRQ_ID);
    
    /* Enable Receiver Interrupts in the LPUART Hardware itself */
    LPUART4->CTRL |= (1 << 21);
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

        c = esp_ring_buffer_pop();
        
        if (c != '\0') {
            
            if(c != '\r' && c != '\n') print_dbg("%c", c);
            if(c == '\n') print_dbg("\\r");
            if(c == '\r') print_dbg("\\r");

            // "OK"?
            if (prev == 'O' && c == 'K') {
                // Read the final '\r' and '\n' to clear the pipe before exiting
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = esp_ring_buffer_pop();
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
                    if (esp_ring_buffer_pop() == '\n') break;
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

    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    print_dbg("[Wi-Fi] Attempting connection... (20 sec timeout)\r\n");
    send_to_esp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (wait_for_esp_ok(20)==false) {
        print_dbg("[Wi-Fi] Failed to connect to Access Point.\r\n");
        return;
    }
    print_dbg("[Wi-Fi] Station connection sequence complete.\r\n");
}

void start_esp_tcp_server(int port) {
    print_dbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections */
    send_to_esp("AT+CIPMUX=1\r\n");
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to enable multiple connections.\r\n");
        return;
    }

    /* This forces the ESP to include the sender's IP address in the +IPD string */
    send_to_esp("AT+CIPDINFO=1\r\n");
    wait_for_esp_ok(3);

    /* Start the Server */
    send_to_esp("AT+CIPSERVER=1,%d\r\n", port);
    if (wait_for_esp_ok(5)==false) {
        print_dbg("[Wi-Fi] Failed to start TCP server.\r\n");
        return;
    }
    
    send_to_esp("AT+CIFSR\r\n");
    wait_for_esp_ok(5);

    print_dbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}


void esp_tcp_client_send(const char* ip, int port, const char* payload) {
    print_dbg("\r\n[Wi-Fi] Sending trigger to %s:%d...\r\n", ip, port);

    // Ensure the ESP8266 is in Multiple Connection Mode
    send_to_esp("AT+CIPMUX=1\r\n");
    wait_for_esp_ok(2);

    // Open Socket #4 as a TCP Client
    send_to_esp("AT+CIPSTART=4,\"TCP\",\"%s\",%d\r\n", ip, port);
    if (wait_for_esp_ok(5) == false) {
        print_dbg("[Wi-Fi] Failed to connect to remote server.\r\n");
        return;
    }

    // Calculate length and send the CIPSEND command
    int len = strlen(payload);
    send_to_esp("AT+CIPSEND=4,%d\r\n", len);
    
    // Wait for the '>' prompt indicating ESP is ready for raw data
    uint64_t targetClockTick = sysctrGetTicks() + (2 * sysctrGetFreq());
    while (sysctrGetTicks() < targetClockTick) {
        /* Clear Overrun errors just in case */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        char c = esp_ring_buffer_pop();
        if (c != '\0') {
            if(c != '\r' && c != '\n') print_dbg("%c", c);
            if(c == '\n') print_dbg("\\n");
            if(c == '\r') print_dbg("\\r");
            
            // Break exactly when the ESP gives the green light
            if (c == '>') {
                print_dbg(">"); 
                break;
            }
        }
        __asm__ volatile("nop"); 
    }

    /* ULTRA-GENTLE CHUNKING --- */
    // Blast the payload safely to avoid ESP8266 "busy s..." UART overflows
    int i = 0;
    char chunk[1024]; // Reduced to 1KB per chunk at a time
    while (payload[i] != '\0') {
        int j = 0;
        while (j < 1023 && payload[i] != '\0') {
            chunk[j++] = payload[i++];
        }
        chunk[j] = '\0';
        
        send_to_esp("%s", chunk);
        
        // Give the ESP8266 300 milliseconds to process the 1024 bytes
        sysctrDelay_ms(300); 
    }
    /* -------------------------------------- */
    
    // Wait for "SEND OK"
    wait_for_esp_ok(3);

    // Close Socket #4
    send_to_esp("AT+CIPCLOSE=4\r\n"); 
    
    // 100ms Blind Flush
    uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 10); 
    while (sysctrGetTicks() < flush_target) {
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }
        esp_ring_buffer_pop(); 
        __asm__ volatile("nop");
    }
    
    print_dbg("[Wi-Fi] Trigger sent successfully.\r\n");
}

void espTCPServerListener_thread(void *arg) {
    char buffer[128];
    char header[64]; 
    char remote_ip[24]; 
    
    // PERSISTENT STATE MACHINE VARIABLES
    int state = 0;     // 0 = Wait for '+', 1 = Read Header, 2 = Read Payload
    int idx = 0;       // Payload buffer index
    int h_idx = 0;     // Header buffer index

    print_dbg("[Wi-Fi] Asynchronous Listener Thread Started.\r\n");

    while(1) {
        // Pop a byte from RAM (returns instantly, doesn't wait for hardware)
        char c = esp_ring_buffer_pop();
        
        // If the buffer is empty, yield the CPU to let ledblink/print100 run!
        if (c == '\0') {
            thread_sleep(5); // Yield CPU for 5ms (or thread_yield() if implemented)
            continue;
        }
        
        // Process the character through the State Machine
        switch (state) {
            case 0: // Waiting for +IPD
                if (c == '+') {
                    state = 1;
                    h_idx = 0;
                    idx = 0;
                }
                break;

            case 1: // Reading Header metadata until ':'
                if (c == ':') {
                    header[h_idx] = '\0'; 
                    state = 2; 
                } else {
                    if (h_idx < 63) header[h_idx++] = c;
                }
                break;

            case 2: // Reading Payload until Newline
                if (c == '\n' || c == '\r') {
                    buffer[idx] = '\0';
                    
                    /* --- Parse the IP from the header --- */
                    int comma_count = 0;
                    int ip_ptr = 0;
                    remote_ip[0] = 'U'; remote_ip[1] = 'n'; remote_ip[2] = 'k'; remote_ip[3] = '\0'; 
                    
                    for (int i = 0; header[i] != '\0'; i++) {
                        if (header[i] == ',') { comma_count++; continue; }
                        
                        if (comma_count == 3) {
                            if (header[i] != '"' && ip_ptr < 23) {
                                remote_ip[ip_ptr++] = header[i];
                            }
                        }
                        if (comma_count == 4) break; 
                    }
                    remote_ip[ip_ptr] = '\0';
                    /* ------------------------------------ */

                    print_dbg("[ESP8266-remote-%s] %s\r\n", remote_ip, buffer);
                    
                    if (strncmp(buffer, "exec ", 5) == 0) {
                        handleCommand(buffer + 5); 
                    }
                    
                    // Reset back to waiting for the next packet
                    state = 0; 
                } else {
                    if (idx < 127) buffer[idx++] = c;
                }
                break;
        }
    }
}
#include <stdbool.h>
#include "SYS_CTR.h"
#include "include/esp8266.h"
#include "include/stdio.h"
#include "include/cli_utility.h"
#include "include/cli.h"
#include "include/string.h"
#include "include/multitasking.h"
#include "GIC.h"
#include "include/irq.h"
#include "include/shared_locks.h"

os_mutex_t esp_transaction_mutex = OS_MUTEX_INITIALIZER;

// ----------------------------------- RING BUFFER ARCHITECTURE and ISR START-----------------------------------

RingBuffer esp_rx_buffer = { .head = 0, .tail = 0 };

// Add this helper function for this kind of thing:
// [ESP8266-response]  0,CONNECT\r\n\r\n+IPD,0,14,192.168.0.100,48984:exec ledblink
// exec ledblink printed by [response] but due to exec we should run ledblink
static void intercept_exec_command(char c) {
    static int state = 0;
    static char cmd_buf[CMD_BUFFER_SIZE] = {0};
    static int cmd_idx = 0;

    if (state == 0 && c == 'e') state = 1;
    else if (state == 1 && c == 'x') state = 2;
    else if (state == 2 && c == 'e') state = 3;
    else if (state == 3 && c == 'c') state = 4;
    else if (state == 4 && c == ' ') { state = 5; cmd_idx = 0; }
    else if (state == 5) {
        if (c == '\n' || c == '\r') {
            cmd_buf[cmd_idx] = '\0';
            if (cmd_idx > 0) {
                handleCommand(cmd_buf);
            }
            state = 0;
        } else {
            if (cmd_idx < 127) cmd_buf[cmd_idx++] = c;
        }
    } else {
        state = (c == 'e') ? 1 : 0; // Reset gracefully
    }
}

char esp_ring_buffer_pop(void) {
    char c = '\0';
    if (esp_rx_buffer.head != esp_rx_buffer.tail) {
        c = esp_rx_buffer.data[esp_rx_buffer.tail];
        esp_rx_buffer.tail = (esp_rx_buffer.tail + 1) % ESP_RX_BUFFER_SIZE;
        
        // Watches every single byte leaving the queue, no matter which thread called pop!
        intercept_exec_command(c); 
    }
    return c;
}

CPUState* lpuart4_rx_isr(CPUState* current_state) {
    uint32_t stat = LPUART4->STAT;

    if (stat & (0xF << 16)) {
        LPUART4->STAT |= (0xF << 16); 
    }

    if (stat & (1 << 21)) {
        char c = (char)(LPUART4->DATA & 0xFF); 
        int next_head = (esp_rx_buffer.head + 1) % ESP_RX_BUFFER_SIZE;
        if (next_head != esp_rx_buffer.tail) {
            esp_rx_buffer.data[esp_rx_buffer.head] = c;
            esp_rx_buffer.head = next_head;
        }
    }
    return current_state; 
}

// -----------------------------------RING BUFFER ARCHITECTURE and ISR END-----------------------------------

void esp_init(void) {
    irq_register(IMX91_LPUART4_IRQ_ID, lpuart4_rx_isr);
    gicEnableInterrupt(IMX91_LPUART4_IRQ_ID);
    LPUART4->CTRL |= (1 << 21);
}

static bool wait_for_esp_ok(uint32_t timeout_sec) {
    char c;
    char prev = 0;
    bool success = false;
    bool timed_out = true;
    
    uint64_t targetClockTick = sysctrGetTicks() + timeout_sec * sysctrGetFreq(); 
    print_dbg("[ESP8266-response] ");
    
    while (sysctrGetTicks() < targetClockTick) {
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        c = esp_ring_buffer_pop();
        
        if (c != '\0') {
            if(c != '\r' && c != '\n') print_dbg("%c", c);
            if(c == '\r') print_dbg("\\r");
            if(c == '\n') print_dbg("\\n");

            if (prev == 'O' && c == 'K') {
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = esp_ring_buffer_pop();
                    if(flush_c != '\r' && flush_c != '\n') print_dbg("%c", flush_c);
                    if(flush_c == '\r') print_dbg("\\r");
                    if(flush_c == '\n') print_dbg("\\n");
                    if (flush_c == '\n') break;
                }
                success = true;
                timed_out = false;
                break;
            }
            
            if (prev == 'O' && c == 'R') {
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = esp_ring_buffer_pop();
                    if(flush_c != '\r' && flush_c != '\n') print_dbg("%c", flush_c);
                    if(flush_c == '\r') print_dbg("\\r");
                    if(flush_c == '\n') print_dbg("\\n");
                    if (flush_c == '\n') break;
                }
                success = false;
                timed_out = false;
                break;
            }

            if (prev == 'I' && c == 'L') {
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = esp_ring_buffer_pop();
                    if(flush_c != '\r' && flush_c != '\n') print_dbg("%c", flush_c);
                    if(flush_c == '\r') print_dbg("\\r");
                    if(flush_c == '\n') print_dbg("\\n");
                    if (flush_c == '\n') break;
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
        print_dbg("[ESP8266-response-Warning] %d sec timeout!\n", timeout_sec);
        return false;
    } 
    print_dbg("\n");
    return success;
}

void esp_reboot(void) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    print_dbg("[ESP-Driver] Sending software reset command to ESP8266...\n");
    send_to_esp("AT+RST\r\n");
    wait_for_esp_ok(2);
    print_dbg("[ESP-Driver] Waiting for ESP8266 silicon to reboot (3 seconds)...\n");

    uint64_t flush_target = sysctrGetTicks() + (3 * sysctrGetFreq()); 
    while (sysctrGetTicks() < flush_target) {
        if (LPUART4->STAT & (0xF << 16)) LPUART4->STAT |= (0xF << 16); 
        esp_ring_buffer_pop(); 
        __asm__ volatile("nop");
    }
    print_dbg("[ESP-Driver] ESP8266 reboot complete.\n");

    mutex_unlock(&esp_transaction_mutex); // UNLOCK
}

void esp_print_status(void) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(3) == false) {
        print_dbg("[ESP-Driver] Failed to communicate.\n");
        mutex_unlock(&esp_transaction_mutex);
        return;
    }

    print_dbg("[ESP-Driver] Operating Mode:\n");
    send_to_esp("AT+CWMODE?\r\n");
    wait_for_esp_ok(3);

    print_dbg("[ESP-Driver] Connected Router (If in Station Mode):\n");
    send_to_esp("AT+CWJAP?\r\n");
    wait_for_esp_ok(5);

    print_dbg("[ESP-Driver] Hosted Network (If in Access Point Mode):\n");
    send_to_esp("AT+CWSAP?\r\n");
    wait_for_esp_ok(3);

    print_dbg("[ESP-Driver] Network Addresses:\n");
    send_to_esp("AT+CIFSR\r\n");
    wait_for_esp_ok(3);

    mutex_unlock(&esp_transaction_mutex); // UNLOCK
}

void esp_init_as_access_point(const char* ssid, const char* password) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    print_dbg("[ESP-Driver] Initializing ESP8266 in Access Point Mode...\n");
    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(5)==false) goto ap_error;

    send_to_esp("AT+CWMODE=2\r\n");
    if (wait_for_esp_ok(5)==false) goto ap_error;

    send_to_esp("AT+CWSAP=\"%s\",\"%s\",6,3\r\n", ssid, password);
    if (wait_for_esp_ok(10)==false) goto ap_error;
    
    print_dbg("[ESP-Driver] Access Point '%s' is now broadcasting.\n", ssid);
    mutex_unlock(&esp_transaction_mutex);
    return;

ap_error:
    print_dbg("[ESP-Driver] Access Point configuration failed.\n");
    mutex_unlock(&esp_transaction_mutex);
}

void esp_init_as_station(const char* ssid, const char* password) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    print_dbg("[ESP-Driver] Initializing ESP8266 in Station Mode...\n");
    send_to_esp("AT\r\n");
    if (wait_for_esp_ok(5)==false) goto sta_error;

    send_to_esp("AT+CWMODE=1\r\n");
    if (wait_for_esp_ok(5)==false) goto sta_error;

    print_dbg("[ESP-Driver] Attempting connection... (20 sec timeout)\n");
    send_to_esp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (wait_for_esp_ok(20)==false) goto sta_error;
    
    print_dbg("[ESP-Driver] Station connection sequence complete.\n");
    mutex_unlock(&esp_transaction_mutex);
    return;

sta_error:
    print_dbg("[ESP-Driver] Station configuration failed.\n");
    mutex_unlock(&esp_transaction_mutex);
}

void esp_start_tcp_server(int port) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    print_dbg("[ESP-Driver] Starting TCP Server on port %d...\n", port);
    send_to_esp("AT+CIPMUX=1\r\n");
    if (wait_for_esp_ok(5)==false) goto tcp_error;

    send_to_esp("AT+CIPDINFO=1\r\n");
    wait_for_esp_ok(3);

    send_to_esp("AT+CIPSERVER=1,%d\r\n", port);
    if (wait_for_esp_ok(5)==false) goto tcp_error;
    
    send_to_esp("AT+CIFSR\r\n");
    wait_for_esp_ok(5);

    print_dbg("[ESP-Driver] TCP Server is running and listening!\n");
    mutex_unlock(&esp_transaction_mutex);
    return;

tcp_error:
    print_dbg("[ESP-Driver] Failed to start TCP server.\n");
    mutex_unlock(&esp_transaction_mutex);
}

void esp_sendto_tcp_clients(const char* ip, int port, const char* payload) {
    mutex_lock(&esp_transaction_mutex); // LOCK

    print_dbg("[ESP-Driver] Sending TCP data to %s:%d...\n", ip, port);
    send_to_esp("AT+CIPMUX=1\r\n");
    wait_for_esp_ok(2);

    send_to_esp("AT+CIPSTART=4,\"TCP\",\"%s\",%d\r\n", ip, port);
    if (wait_for_esp_ok(5) == false) {
        print_dbg("[ESP-Driver] Failed to connect to remote server.\n");
        send_to_esp("AT+CIPCLOSE=4\r\n"); 
        wait_for_esp_ok(3);
        mutex_unlock(&esp_transaction_mutex); // Safe Error Release
        return;
    }

    int len = strlen(payload);
    send_to_esp("AT+CIPSEND=4,%d\r\n", len);
    
    print_dbg("[ESP8266-response] ");
    uint64_t targetClockTick = sysctrGetTicks() + (2 * sysctrGetFreq());
    while (sysctrGetTicks() < targetClockTick) {
        if (LPUART4->STAT & (0xF << 16)) LPUART4->STAT |= (0xF << 16); 
        char c = esp_ring_buffer_pop();
        if (c != '\0') {
            if(c != '\r' && c != '\n') print_dbg("%c", c);
            if(c == '\n') print_dbg("\\n");
            if(c == '\r') print_dbg("\\r");
            if (c == '>') break;
        }
        __asm__ volatile("nop"); 
    }
    print_dbg("\n");

    int i = 0;
    char chunk[1024];
    while (payload[i] != '\0') {
        int j = 0;
        while (j < 1024 && payload[i] != '\0') {
            chunk[j++] = payload[i++];
        }
        chunk[j] = '\0';
        send_to_esp("%s", chunk);
    }
    
    wait_for_esp_ok(3);
    send_to_esp("AT+CIPCLOSE=4\r\n"); 
    wait_for_esp_ok(3);

    uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 10); 
    while (sysctrGetTicks() < flush_target) {
        if (LPUART4->STAT & (0xF << 16)) LPUART4->STAT |= (0xF << 16); 
        esp_ring_buffer_pop(); 
        __asm__ volatile("nop");
    }
    
    print_dbg("[ESP-Driver] TCP data sent successfully.\n");
    mutex_unlock(&esp_transaction_mutex); // UNLOCK
}

void espTCPServerListener_thread(void *arg) {
    char buffer[128];
    char header[64]; 
    char remote_ip[24]; 
    
    int state = 0;     
    int idx = 0;       
    int h_idx = 0;     

    while(1) {
        // If another thread (like statistics) is busy sending an AT command,
        // the listener thread will back off and yield the CPU so it doesn't
        // steal the "OK" bytes from the ring buffer!
        if (esp_transaction_mutex.value == 0) { 
            thread_sleep(5); 
            continue; 
        }

        char c = esp_ring_buffer_pop();
        
        if (c == '\0') {
            thread_sleep(5); 
            continue;
        }
        
        switch (state) {
            case 0:
                if (c == '+') {
                    state = 1;
                    h_idx = 0;
                    idx = 0;
                }
                break;

            case 1: 
                if (c == ':') {
                    header[h_idx] = '\0'; 
                    state = 2; 
                } else {
                    if (h_idx < 63) header[h_idx++] = c;
                }
                break;

            case 2: 
                if (c == '\n' || c == '\r') {
                    buffer[idx] = '\0';
                    
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

                    print_dbg("\n[ESP8266-Remote-%s] %s", remote_ip, buffer);
                    
                    if (strncmp(buffer, "exec ", 5) == 0) {
                        handleCommand(buffer + 5); 
                    }
                    state = 0; 
                } else {
                    if (idx < 127) buffer[idx++] = c;
                }
                break;
        }
    }
}
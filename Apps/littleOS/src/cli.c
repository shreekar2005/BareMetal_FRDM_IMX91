#include <stdint.h>
#include "cli.h"
#include "LPUART.h"
#include "kmultitasking.h"

#define ASCII_CTRL_C 0x03
#define ASCII_BACKSPACE 0x08
#define ASCII_DEL 0x7F

// --- STRING UTILITIES ---
int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int my_strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int my_atoi(const char *str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

// --- HARDWARE ABSTRACTIONS ---

// Blocks until a character is typed, but yields CPU to other threads while waiting
char yield_getchar(void) {
    char c;
    while(1) {
        c = uart_getchar_nonblocking(LPUART1);
        if (c != '\0') return c;
        os_yield(); 
    }
}

// Performs a Soft Hardware Reset by jumping back to the U-Boot entry point!
void system_reboot(void) {
    uart_print_string(LPUART1, "\r\n[System] Performing Soft Reboot...\r\n");
    
    // Create a function pointer to physical address 0x80000000 and call it
    void (*reset_vector)(void) = (void*)0x80000000;
    reset_vector();
}

// --- THE CLI THREAD ---

void input_thread(void* arg) {
    char cmd[128];
    int cmd_idx = 0;
    
    uart_print_string(LPUART1, "\r\n> ");

    while(!os_halt) {
        char c = yield_getchar();
        
        // 1. Thread Cancellation (Ctrl+C)
        if (c == ASCII_CTRL_C) {
            run_led = false;
            print_active = false;
            print_count = 0;
            uart_print_string(LPUART1, "^C\r\n[System] All active background threads stopped.\r\n> ");
            cmd_idx = 0;
            continue;
        }
        
        // 2. Backspace
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cmd_idx > 0) {
                cmd_idx--;
                uart_print_string(LPUART1, "\b \b");
            }
            continue;
        }

        // 3. Command Execution
        if (c == '\r' || c == '\n') {
            cmd[cmd_idx] = '\0'; 
            
            if (cmd_idx > 0) {
                // LED Command
                if (my_strcmp(cmd, "led") == 0) {
                    run_led = !run_led;
                    uart_print_string(LPUART1, "\r\n[System] LED state toggled.");
                } 
                // Reboot Commands
                else if (my_strcmp(cmd, "q") == 0 || my_strcmp(cmd, "exit") == 0 || my_strcmp(cmd, "reboot") == 0) {
                    system_reboot();
                }
                // Shutdown Command
                else if (my_strcmp(cmd, "shutdown") == 0) {
                    uart_print_string(LPUART1, "\r\n[System] Shutting down OS...\r\n");
                    os_halt = true;
                    return; // Kills the CLI thread
                }
                // Help Command
                else if (my_strcmp(cmd, "help") == 0 || my_strcmp(cmd, "?") == 0) {
                    uart_print_string(LPUART1, "\r\n[Help] Available Commands:\r\n");
                    uart_print_string(LPUART1, "  led               - Start/Stop background LED blinking\r\n");
                    uart_print_string(LPUART1, "  print text n      - Print 'text' n times with 0.5s delay\r\n");
                    uart_print_string(LPUART1, "  reboot, q, exit   - Soft reboot the OS\r\n");
                    uart_print_string(LPUART1, "  shutdown          - Halt the CPU\r\n");
                    uart_print_string(LPUART1, "  Ctrl+C            - Stop active background threads\r\n");
                }
                // Print Command
                else if (my_strncmp(cmd, "print ", 6) == 0) {
                    if (print_active) {
                        uart_print_string(LPUART1, "\r\n[System] Error: Print worker is busy! Press Ctrl+C to stop it.");
                    } else {
                        int i = 6;
                        int buf_idx = 0;
                        while(cmd[i] == ' ') i++; 
                        if (cmd[i] == '"') {
                            i++; 
                            while(cmd[i] != '\0' && cmd[i] != '"' && buf_idx < 127) print_buffer[buf_idx++] = cmd[i++];
                            if (cmd[i] == '"') i++; 
                        } else {
                            while(cmd[i] != '\0' && cmd[i] != ' ' && buf_idx < 127) print_buffer[buf_idx++] = cmd[i++];
                        }
                        print_buffer[buf_idx] = '\0';
                        while(cmd[i] == ' ') i++; 
                        
                        print_count = my_atoi(&cmd[i]);
                        if (print_count <= 0) print_count = 1; 
                        
                        print_active = true;
                        uart_print_string(LPUART1, "\r\n[System] Print job dispatched to background.");
                    }
                }
                else {
                    uart_print_string(LPUART1, "\r\n[System] Unknown command. Type 'help' for options.");
                }
            }
            
            cmd_idx = 0;
            if (!os_halt) uart_print_string(LPUART1, "\r\n> ");
        } 
        // 4. Standard Typing (ASCII Shield)
        else if (c >= 32 && c <= 126) {
            if (cmd_idx < 127) {
                cmd[cmd_idx++] = c;
                uart_putchar(LPUART1, c);
            }
        }
    }
}
#include <stdint.h>
#include "include/cli.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/stdio.h"
#include "LPUART.h"
#include "include/multitasking.h"

#define ASCII_CTRL_C 0x03
#define ASCII_BACKSPACE 0x08
#define ASCII_DEL 0x7F

void input_thread(void* arg) {
    char cmd[128];
    int cmd_idx = 0;
    
    printf("\r\n> ");

    while(!os_halt) {
        char c;
        while(1) {
            c = uart_getchar_nonblocking(LPUART1);
            if (c != '\0') break;
        }
        
        if (c == ASCII_CTRL_C) {
            print_count = 0;
            printf("^C\r\n[System] All active background threads stopped.\r\n> ");
            cmd_idx = 0;
            continue;
        }
        
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cmd_idx > 0) {
                cmd_idx--;
                printf("\b \b");
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            cmd[cmd_idx] = '\0'; 
            
            if (cmd_idx > 0) {
                if (my_strcmp(cmd, "help") == 0 || my_strcmp(cmd, "?") == 0) {
                    printf("\r\n[Help] Available Commands:\r\n");
                    printf("  reboot        - Hardware reboot\r\n");
                    printf("  shutdown      - Power off the system (not working properly currently)\r\n");
                    printf("  ledblink      - Blink the hardware LED once (led_blink_thread)\r\n");
                    printf("  print text n  - Print 'text' n times with time slicing (print_thread)\r\n");
                    printf("  printa text n - Atomic Print 'text' n times locking the CPU (atomic_print_thread)\r\n");
                    printf("  Ctrl+C        - Stop active background threads\r\n");
                }
                else if (my_strcmp(cmd, "reboot") == 0) {
                    system_reboot();
                }
                else if (my_strcmp(cmd, "shutdown") == 0) {
                    system_poweroff(); 
                }
                else if (my_strcmp(cmd, "ledblink") == 0) {
                    os_thread_start(led_blink_thread_id); 
                    printf("\r\n[System] LED blink dispatched.");
                }
                else if (my_strncmp(cmd, "print ", 6) == 0) {
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
                    os_thread_start(print_thread_id); 
                    printf("\r\n[System] Print job dispatched to background.");
                }
                else if (my_strncmp(cmd, "printa ", 7) == 0) {
                    int i = 7;
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
                    
                    os_thread_start(atomic_print_thread_id); 
                    printf("\r\n[System] Atomic print job dispatched to background.");
                }
                else {
                    printf("\r\n[System] Unknown command. Type 'help' for options.");
                }
            }
            
            cmd_idx = 0;
            if (!os_halt) printf("\r\n> ");
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_idx < 127) {
                cmd[cmd_idx++] = c;
                printf("%c", c);
            }
        }
    }
}
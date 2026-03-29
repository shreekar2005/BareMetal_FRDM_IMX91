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

static int get_flag_int(const char* str, const char* flag, int default_val) {
    const char* pos = my_strstr(str, flag);
    if (pos) {
        pos += my_strlen(flag);
        while (*pos == ' ') pos++;
        return my_atoi(pos);
    }
    return default_val;
}

void input_thread(void* arg) {
    char cmd[128];
    int cmd_idx = 0;
    
    printf("\n> ");

    while(!os_halt) {
        char c;
        while(1) {
            c = uart_getchar_nonblocking(LPUART1);
            if (c != '\0') break;
        }
        
        if (c == ASCII_CTRL_C) {
            // brutally kill all background tasks instantly
            os_kill_thread(led_blink_thread_id);
            os_kill_thread(print100X_thread_id);
            os_kill_thread(print100o_thread_id);
            os_kill_thread(atomic_print100A_thread_id);
            os_kill_thread(echo_thread_id);

            printf("^C\n[System] All active background tasks forcefully killed.\n> ");
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
                    printf("\n[Help] Available Commands:\n");
                    printf(" stat              - View RTOS Task Manager\n");
                    printf(" clear             - Clear the terminal screen\n");
                    printf(" reboot            - Hardware reboot\n");
                    printf(" Ctrl+C            - To stop all threads\n");
                    printf(" sched [rr|pri|edf]- Change RTOS scheduler algorithm\n");
                    printf("\nTo start task :\n");
                    printf("   <taskname> -n <how many times to start again> -per <period> -pri <priority> -d <deadline>\n");
                    printf(" Defaults: -n 1, -per 0, -pri 128, -d -1\n");
                    printf(" Tasks:\n");
                    printf("   ledblink          - Blink LED\n");
                    printf("   echo \"text\"       - Print text on console\n");
                    printf("   print100X         - Print 100 X characters\n");
                    printf("   print100o         - Print 100 o characters\n");
                    printf("   aprint100A        - Print 100 A characters atomically (no interleaving)\n");
                }
                else if (my_strcmp(cmd, "stat") == 0) {
                    print_stat();
                }
                else if (my_strcmp(cmd, "clear") == 0) {
                    clear_terminal();
                }
                else if (my_strcmp(cmd, "reboot") == 0) {
                    system_reboot();
                }
                else if (my_strcmp(cmd, "shutdown") == 0) {
                    system_poweroff(); 
                }
                else if (my_strcmp(cmd, "sched rr") == 0) {
                    os_set_scheduling_algo(SCHED_RR);
                    printf("\n[System] Switched to Round Robin scheduling.");
                }
                else if (my_strcmp(cmd, "sched pri") == 0) {
                    os_set_scheduling_algo(SCHED_PRIORITY);
                    printf("\n[System] Switched to Fixed Priority scheduling.");
                }
                else if (my_strcmp(cmd, "sched edf") == 0) {
                    os_set_scheduling_algo(SCHED_EDF);
                    printf("\n[System] Switched to Earliest Deadline First scheduling.");
                }
                else {
                    int target_id = -1;
                    bool is_print = false;
                    int i = 0;
                    
                    if (my_strncmp(cmd, "ledblink", 8) == 0) {
                        target_id = led_blink_thread_id;
                        i = 8;
                    } 
                    else if (my_strncmp(cmd, "echo ", 5) == 0) {
                        target_id = echo_thread_id;
                        is_print = true;
                        i = 5;
                    } 
                    else if (my_strncmp(cmd, "print100X", 9) == 0) {
                        target_id = print100X_thread_id;
                        i = 9;
                    } 
                    else if (my_strncmp(cmd, "print100o", 9) == 0) {
                        target_id = print100o_thread_id;
                        i = 9;
                    }
                    else if (my_strncmp(cmd, "aprint100A", 10) == 0) {
                        target_id = atomic_print100A_thread_id;
                        is_print = true;
                        i = 10;
                    } 
                    
                    if (target_id != -1) {
                        if (is_print) {
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
                        }
                        
                        int n = get_flag_int(cmd, "-n ", 1);
                        int per = get_flag_int(cmd, "-per ", 0);
                        int pri = get_flag_int(cmd, "-pri ", 128);
                        int d = get_flag_int(cmd, "-d ", -1);
                        
                        os_set_thread_rtos(target_id, pri, d, per, n);
                        os_thread_start(target_id); 
                        
                        printf("\n[System] Task dispatched (n:%d per:%dms pri:%d d:%dms).", n, per, pri, d);
                    } else {
                        printf("\n[System] Unknown command. Type 'help' for options.");
                    }
                }
            }
            
            cmd_idx = 0;
            if (!os_halt) printf("\n> ");
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_idx < 127) {
                cmd[cmd_idx++] = c;
                printf("%c", c);
            }
        }
    }
}
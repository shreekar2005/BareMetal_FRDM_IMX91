#include <stdint.h>
#include "include/cli.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/stdio.h"
#include "LPUART.h"
#include "include/multitasking.h"
#include "include/autotasks.h"

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
    
    printdbg("\n> ");

    while(!os_halt) {
        char c;
        while(1) {
            c = lpuart_getchar_nonblocking(LPUART1);
            if (c != '\0') break;
        }
        
        if (c == ASCII_CTRL_C) {
            // LOOP THROUGH AUTO-REGISTRY AND KILL EVERYTHING
            for (int i = 0; i < num_autotasks; i++) {
                os_kill_thread(*(autotasks[i].id_ptr));
            }
            printdbg("^C\n[System] All active background tasks forcefully killed.\n> ");
            cmd_idx = 0;
            continue;
        }
        
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cmd_idx > 0) {
                cmd_idx--;
                printdbg("\b \b");
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            cmd[cmd_idx] = '\0'; 
            
            if (cmd_idx > 0) {
                if (my_strcmp(cmd, "help") == 0 || my_strcmp(cmd, "?") == 0) {
                    print_help();
                }
                else if (my_strcmp(cmd, "stat") == 0) {
                    print_stat();
                }
                else if (my_strcmp(cmd, "clear") == 0) {
                    clear_terminal();
                }
                else if (my_strcmp(cmd, "reboot") == 0 || my_strcmp(cmd, "restart") == 0 || my_strcmp(cmd, "reset") == 0) {
                    system_reboot();
                }
                else if (my_strcmp(cmd, "shutdown") == 0 || my_strcmp(cmd, "poweroff") == 0) {
                    system_poweroff(); 
                }
                else if (my_strncmp(cmd, "sched ", 6) == 0) {
                    if (my_strcmp(cmd, "sched rr") == 0) os_set_scheduling_algo(SCHED_RR);
                    else if (my_strcmp(cmd, "sched pri") == 0) os_set_scheduling_algo(SCHED_PRIORITY);
                    else if (my_strcmp(cmd, "sched edf") == 0) os_set_scheduling_algo(SCHED_EDF);
                    printdbg("\n[System] Scheduler algorithm changed.");
                }
                else {
                    int target_id = -1;
                    int i = 0;
                    
                    // DYNAMIC TASK MATCHER
                    for (int t = 0; t < num_autotasks; t++) {
                        int len = my_strlen(autotasks[t].cmd_string);
                        if (my_strncmp(cmd, autotasks[t].cmd_string, len) == 0) {
                            // Ensure it's an exact match or followed by space
                            if (cmd[len] == ' ' || cmd[len] == '\0') {
                                target_id = *(autotasks[t].id_ptr);
                                i = len;
                                break;
                            }
                        }
                    }
                    
                    if (target_id != -1) {
                        // GLOBAL STRING EXTRACTOR (works for any command!)
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
                        
                        // PARSE FLAGS
                        int n = get_flag_int(cmd, "-n ", 1);
                        int per = get_flag_int(cmd, "-per ", 0);
                        int pri = get_flag_int(cmd, "-pri ", 128);
                        int d = get_flag_int(cmd, "-d ", -1);
                        
                        os_set_thread_rtos(target_id, pri, d, per, n);
                        os_thread_start(target_id); 
                        
                        printdbg("\n[System] Task dispatched (n:%d per:%dms pri:%d d:%dms).", n, per, pri, d);
                    } else {
                        printdbg("\n[System] Unknown command. Type 'help' for options.");
                    }
                }
            }
            
            cmd_idx = 0;
            if (!os_halt) printdbg("\n> ");
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_idx < 127) {
                cmd[cmd_idx++] = c;
                printdbg("%c", c);
            }
        }
    }
}
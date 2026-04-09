#include "include/cli_utility.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include <stdint.h>
#include "include/autotasks.h"
#include "include/string.h"

extern volatile char print_buffer[128]; /**< buffer defined in cli.c */

static int get_flag_int(const char* str, const char* flag, int default_val) {
    const char* pos = my_strstr(str, flag);
    if (pos) {
        pos += my_strlen(flag);
        while (*pos == ' ') pos++;
        return my_atoi(pos);
    }
    return default_val;
}

void handleCommand(const char* cmd) {
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

void system_reboot(void) {
    printdbg("\n[System] TRIGGERING HARDWARE WATCHDOG RESET...\n");
    __asm__ volatile("msr daifset, #2");

    volatile uint32_t* wdog_cs    = (volatile uint32_t*)(0x442D0000 + 0x00);
    volatile uint32_t* wdog_cnt   = (volatile uint32_t*)(0x442D0000 + 0x04);
    volatile uint32_t* wdog_toval = (volatile uint32_t*)(0x442D0000 + 0x08);

    *wdog_cnt = 0xD928C520;
    *wdog_toval = 0x00000000;
    *wdog_cs = 0x00002920; 

    while(1) { __asm__ volatile("nop"); }
}

// NOT SHUTTING DOWN ACTUAL HARDWARE, JUST SIMULATING POWER-OFF BY HALTING THE SYSTEM AND LETTING THE USER KNOW
void system_poweroff(void) {
    printdbg("\n[System] SENDING POWER-DOWN SIGNAL TO PMIC...\n");
    __asm__ volatile("msr daifset, #2"); 
    volatile uint32_t* snvs_lpcr = (volatile uint32_t*)(0x44470000 + 0x38);
    *snvs_lpcr |= (1 << 5) | (1 << 6);
    while(1) { __asm__ volatile("wfi"); }
}

void clear_terminal(void) {
    printdbg("\033[2J\033[H");
}

void print_help(void){
    printdbg("\n[Help] Available Commands:\n");
    printdbg(" stat                 - View RTOS Task Manager\n");
    printdbg(" clear                - Clear the terminal screen\n");
    printdbg(" reboot/restart/reset - Hardware reboot\n");
    printdbg(" shutdown/poweroff    - Hardware poweroff (NOT SHUTTING DOWN HARDWARE!!!)\n");
    printdbg(" Ctrl+C               - To stop all threads\n");
    printdbg(" sched [rr|pri|edf]   - Change RTOS scheduler algorithm\n");
    printdbg("\nDynamic Tasks (Auto-Loaded from tasks/):\n");
    for (int i = 0; i < num_autotasks; i++) {
        printdbg("   %-16s - %s\n", autotasks[i].cmd_string, autotasks[i].display_name);
    }
    printdbg(" Defaults: -n 1, -per 0, -pri 128, -d -1\n");
    printdbg(" Syntax: <taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>\n");
}

void print_stat(void) {
    const char* algo_name = "UNKNOWN";
    if (current_algo == SCHED_RR) algo_name = "Round Robin (RR)";
    else if (current_algo == SCHED_PRIORITY) algo_name = "Fixed Priority (PRI)";
    else if (current_algo == SCHED_EDF) algo_name = "Earliest Deadline First (EDF)";

    printdbg("\n[System] Active Scheduler: %s\n", algo_name);
    printdbg("\nID | Name             | State   | Pri | Deadln | Period | Execs     | Turnaround Time");
    printdbg("\n---------------------------------------------------------------------------------------");
    
    for (int i = 0; i < num_threads; i++) {
        printdbg("\n%2d | %-16s | %-7s | %3d | %6d | %6d | ", 
            i, threads[i].name, threads[i].active ? "RUNNING" : "SLEEP",
            threads[i].priority, threads[i].deadline_offset_ms, threads[i].period_ms);
        
        if (threads[i].executions_target == -1) {
            printdbg("%4d/INF  | ", threads[i].executions_done);
        } else {
            printdbg("%4d/%-4d | ", threads[i].executions_done, threads[i].executions_target);
        }
        
        printdbg("%9d ms", threads[i].last_exec_time_ms);
    }
}

/** @brief used to print fatal errors */
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
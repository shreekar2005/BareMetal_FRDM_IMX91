#include <stdint.h>
#include "include/cli_utility.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include "include/autotasks.h"
#include "include/string.h"
#include "include/datetime.h"

extern volatile char print_buffer[128]; /**< buffer defined in cli.c */

static int get_flag_int(const char* str, const char* flag, int default_val) {
    const char* pos = strstr(str, flag);
    if (pos) {
        pos += strlen(flag);
        while (*pos == ' ') pos++;
        return atoi(pos);
    }
    return default_val;
}

void handleCommand(const char* cmd) {
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        print_help();
    }
    else if (strcmp(cmd, "killall") == 0) {
        for (int i = 0; i < numAutotasks; i++) {
            os_kill_thread(*(autotasks[i].id_ptr));
        }
        print_dbg("[CLI-Thread] All active background tasks forcefully killed.\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        clear_terminal();
    }
    else if (strcmp(cmd, "reboot") == 0 || strcmp(cmd, "restart") == 0 || strcmp(cmd, "reset") == 0) {
        system_reboot();
    }
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "poweroff") == 0) {
        system_poweroff(); 
    }
    else if (strncmp(cmd, "sched ", 6) == 0) {
        if (strcmp(cmd, "sched rr") == 0) os_set_scheduling_algo(SCHED_RR);
        else if (strcmp(cmd, "sched pri") == 0) os_set_scheduling_algo(SCHED_PRIORITY);
        else if (strcmp(cmd, "sched edf") == 0) os_set_scheduling_algo(SCHED_EDF);
        else {
            print_dbg("[CLI-Thread] Invalid scheduler. Usage: sched [rr|pri|edf]\n");
            return;
        }
    }
    else if (strncmp(cmd, "datetime", 8) == 0) {
        // PASS THE RAW CMD POINTER DIRECTLY INSTEAD OF RELYING ON print_buffer
        datetime_handlecmd(cmd);
    }
    else {
        int targetID = -1;
        int i = 0;
        
        // DYNAMIC TASK MATCHER
        for (int t = 0; t < numAutotasks; t++) {
            int len = strlen(autotasks[t].cmd_string);
            if (strncmp(cmd, autotasks[t].cmd_string, len) == 0) {
                // Ensure it's an exact match or followed by space
                if (cmd[len] == ' ' || cmd[len] == '\0') {
                    targetID = *(autotasks[t].id_ptr);
                    i = len;
                    break;
                }
            }
        }
        
        if (targetID != -1) {
            // GLOBAL STRING EXTRACTOR (works for any command!)
            int print_buffer_idx = 0;
            while(cmd[i] == ' ') i++; 
            if (cmd[i] == '"') {
                i++; 
                while(cmd[i] != '\0' && cmd[i] != '"' && print_buffer_idx < 127) print_buffer[print_buffer_idx++] = cmd[i++];
                if (cmd[i] == '"') i++; 
            } else {
                while(cmd[i] != '\0' && print_buffer_idx < 127) print_buffer[print_buffer_idx++] = cmd[i++];
            }
            print_buffer[print_buffer_idx] = '\0';
            
            // PARSE FLAGS
            int n = get_flag_int(cmd, "-n ", 1);
            int per = get_flag_int(cmd, "-per ", 0);
            int pri = get_flag_int(cmd, "-pri ", 128);
            int d = get_flag_int(cmd, "-d ", -1);
            
            os_set_thread_rtos(targetID, pri, d, per, n);
            os_thread_start(targetID); 
            
        } else {
            print_dbg("[CLI-Thread] Unknown command. Type 'help' for options.\n");
        }
    }
}

void system_reboot(void) {
    print_dbg("[CLI-Thread] TRIGGERING HARDWARE WATCHDOG RESET...\n\n");
    __asm__ volatile("msr daifset, #2");

    volatile uint32_t* wdog_cs    = (volatile uint32_t*)(0x442D0000 + 0x00);
    volatile uint32_t* wdog_cnt   = (volatile uint32_t*)(0x442D0000 + 0x04);
    volatile uint32_t* wdog_toval = (volatile uint32_t*)(0x442D0000 + 0x08);

    *wdog_cnt = 0xD928C520;
    *wdog_toval = 0x00000000;
    *wdog_cs = 0x00002920; 

    while(1) { __asm__ volatile("nop"); }
}

// NOT SHUTTING DOWN ACTUAL HARDWARE, JUST SIMULATING POWER-OFF BY HALTING THE CLI AND LETTING THE USER KNOW
void system_poweroff(void) {
    print_dbg("[CLI-Thread] SENDING POWER-DOWN SIGNAL TO PMIC...\n");
    __asm__ volatile("msr daifset, #2"); 
    volatile uint32_t* snvs_lpcr = (volatile uint32_t*)(0x44470000 + 0x38);
    *snvs_lpcr |= (1 << 5) | (1 << 6);
    while(1) { __asm__ volatile("wfi"); }
}

void clear_terminal(void) {
    print_dbg("\033[2J\033[H");
}

void print_help(void){
    print_dbg("[CLI-Thread] Available Commands:\n");
    print_dbg("[CLI-Thread]   killall (or Ctrl+C)  - Stop all active threads immediately\n");
    print_dbg("[CLI-Thread]   clear                - Clear the terminal screen\n");
    print_dbg("[CLI-Thread]   reboot/restart/reset - Hardware reboot\n");
    print_dbg("[CLI-Thread]   shutdown/poweroff    - Hardware poweroff (NOT SHUTTING DOWN HARDWARE!!!)\n");
    print_dbg("[CLI-Thread]   sched [rr|pri|edf]   - Change RTOS scheduler algorithm\n");
    print_dbg("[CLI-Thread]   datetime             - System RTC time management\n");
    print_dbg("[CLI-Thread]\n");
    print_dbg("[CLI-Thread] Dynamic Tasks (Auto-Loaded from tasks/):\n");
    for (int i = 0; i < numAutotasks; i++) {
        print_dbg("[CLI-Thread]    %-16s - %s\n", autotasks[i].cmd_string, autotasks[i].display_name);
    }
    print_dbg("[CLI-Thread]  Defaults: -n 1, -per 0, -pri 128, -d -1\n");
    print_dbg("[CLI-Thread]  Syntax: <taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>\n");
}

/** @brief used to print fatal errors */
void os_fatal_error(uint64_t esr, uint64_t elr, uint64_t far, uint64_t type) {
    print_dbg("\n\n=================================\n");
    print_dbg("!!! FATAL CPU EXCEPTION !!!\n");
    if (type == 0) print_dbg("Type: Synchronous Exception\n");
    if (type == 1) print_dbg("Type: Unhandled IRQ Trap\n");
    if (type == 2) print_dbg("Type: FIQ\n");
    if (type == 3) print_dbg("Type: SError (System Bus Fault)\n");

    print_dbg("ESR_EL2 (Reason) : 0x%016llX\n", esr);
    print_dbg("ELR_EL2 (Address) : 0x%016llX\n", elr);
    print_dbg("FAR_EL2 (Memory) : 0x%016llX\n", far);
    print_dbg("System Halted.\n=================================\n");
    while(1) { __asm__ volatile("wfi"); }
}
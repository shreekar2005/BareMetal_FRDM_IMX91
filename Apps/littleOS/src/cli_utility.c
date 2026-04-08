#include "include/cli_utility.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include <stdint.h>
#include "include/autotasks.h"

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
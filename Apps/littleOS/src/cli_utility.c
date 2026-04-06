#include "include/cli_utility.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include <stdint.h>
#include "include/autotasks.h"

void system_reboot(void) {
    printf("\n[System] TRIGGERING HARDWARE WATCHDOG RESET...\n");
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
    printf("\n[System] SENDING POWER-DOWN SIGNAL TO PMIC...\n");
    __asm__ volatile("msr daifset, #2"); 
    volatile uint32_t* snvs_lpcr = (volatile uint32_t*)(0x44470000 + 0x38);
    *snvs_lpcr |= (1 << 5) | (1 << 6);
    while(1) { __asm__ volatile("wfi"); }
}

void clear_terminal(void) {
    printf("\033[2J\033[H");
}

void print_help(void){
    printf("\n[Help] Available Commands:\n");
    printf(" stat                 - View RTOS Task Manager\n");
    printf(" clear                - Clear the terminal screen\n");
    printf(" reboot/restart/reset - Hardware reboot\n");
    printf(" shutdown/poweroff    - Hardware poweroff (NOT SHUTTING DOWN HARDWARE!!!)\n");
    printf(" Ctrl+C               - To stop all threads\n");
    printf(" sched [rr|pri|edf]   - Change RTOS scheduler algorithm\n");
    printf("\nDynamic Tasks (Auto-Loaded from tasks/):\n");
    for (int i = 0; i < num_autotasks; i++) {
        printf("   %-16s - %s\n", autotasks[i].cmd_string, autotasks[i].display_name);
    }
    printf(" Defaults: -n 1, -per 0, -pri 128, -d -1\n");
    printf(" Syntax: <taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>\n");
}

void print_stat(void) {
    const char* algo_name = "UNKNOWN";
    if (current_algo == SCHED_RR) algo_name = "Round Robin (RR)";
    else if (current_algo == SCHED_PRIORITY) algo_name = "Fixed Priority (PRI)";
    else if (current_algo == SCHED_EDF) algo_name = "Earliest Deadline First (EDF)";

    printf("\n[System] Active Scheduler: %s\n", algo_name);
    printf("\nID | Name             | State   | Pri | Deadln | Period | Execs     | Turnaround Time");
    printf("\n---------------------------------------------------------------------------------------");
    
    for (int i = 0; i < num_threads; i++) {
        printf("\n%2d | %-16s | %-7s | %3d | %6d | %6d | ", 
            i, threads[i].name, threads[i].active ? "RUNNING" : "SLEEP",
            threads[i].priority, threads[i].deadline_offset_ms, threads[i].period_ms);
        
        if (threads[i].executions_target == -1) {
            printf("%4d/INF  | ", threads[i].executions_done);
        } else {
            printf("%4d/%-4d | ", threads[i].executions_done, threads[i].executions_target);
        }
        
        printf("%9d ms", threads[i].last_exec_time_ms);
    }
}
#include "include/cli_utility.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include <stdint.h>

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

void system_poweroff(void) {
    printf("\n[System] SENDING POWER-DOWN SIGNAL TO PMIC...\n");
    __asm__ volatile("msr daifset, #2");
    
    volatile uint32_t* snvs_lpcr = (volatile uint32_t*)(0x44470000 + 0x38);
    *snvs_lpcr |= (1 << 5);

    while(1) { __asm__ volatile("wfi"); }
}

void clear_terminal(void) {
    printf("\033[2J\033[H");
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
#include "include/cli_utility.h"
#include "include/stdio.h"
#include <stdint.h>

void system_reboot(void) {
    printf("\r\n[System] TRIGGERING HARDWARE WATCHDOG RESET...\r\n");
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
    printf("\r\n[System] SENDING POWER-DOWN SIGNAL TO PMIC...\r\n");
    __asm__ volatile("msr daifset, #2");
    
    volatile uint32_t* snvs_lpcr = (volatile uint32_t*)(0x44470000 + 0x38);
    *snvs_lpcr |= (1 << 5);

    while(1) { __asm__ volatile("wfi"); }
}

void clear_terminal(void) {
    // \033[2J clears the screen, \033[H moves the cursor to the top left
    printf("\033[2J\033[H");
}
#ifndef _CLI_H
#define _CLI_H

#include <stdbool.h>

// Extern declarations allow cli.c to control variables living in main.c
extern volatile bool run_led;
extern volatile bool os_halt;

extern volatile char print_buffer[128];
extern volatile int print_count;
extern volatile bool print_active;

// The entry point for the CLI thread
void input_thread(void* arg);

#endif
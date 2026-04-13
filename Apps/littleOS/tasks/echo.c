// Task_Name : DBG ECHO

#include "include/stdio.h"

extern volatile char print_buffer[128]; /**< buffer defined in cli.c */

void echo_thread(void* arg) {
    // print_buffer is safely accessed via include/cli.h
    print_dbg("[ECHO] %s", (const char*)print_buffer);
}
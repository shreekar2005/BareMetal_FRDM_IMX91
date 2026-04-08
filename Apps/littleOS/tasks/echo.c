// Task_Name : Console Echo

#include "include/cli.h"
#include "include/stdio.h"

void echo_thread(void* arg) {
    // print_buffer is safely accessed via include/cli.h
    printdbg("%s", (const char*)print_buffer);
}
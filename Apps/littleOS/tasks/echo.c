// Task_Name : DBG ECHO

#include <stddef.h>
#include "include/stdio.h"

void echo_thread(void* arg) {
    char* cmd_string = (char*)arg;
    if (cmd_string == NULL) return; 
    print_dbg("[ECHO-Thread] %s\n", cmd_string);
}
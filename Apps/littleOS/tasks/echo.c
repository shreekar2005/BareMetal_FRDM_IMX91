// Task_Name : DBG ECHO

#include <stddef.h>
#include "include/stdio.h"

void echo_thread(void* arg) {
    char* cmd_string = (char*)arg;
    
    // Safety check
    if (cmd_string == NULL) return; 

    // The argument buffer already contains everything typed after "echo "
    print_dbg("[ECHO-Thread] %s\n", cmd_string);
}
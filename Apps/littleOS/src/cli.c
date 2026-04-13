#include <stdint.h>
#include "LPUART.h"
#include "include/cli.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include "include/autotasks.h"

#define ASCII_CTRL_C 0x03
#define ASCII_BACKSPACE 0x08
#define ASCII_DEL 0x7F

volatile char print_buffer[128];

void input_thread(void* arg) {
    char cmd[128];
    int cmd_idx = 0;
    
    print_dbg("\n[CLI] > ");

    while(1) {
        char c;
        while(1) {
            c = lpuartGetCharNonBlocking(LPUART1);
            if (c != '\0') break;
        }
        
        if (c == ASCII_CTRL_C) {
            // LOOP THROUGH AUTO-REGISTRY AND KILL EVERYTHING
            for (int i = 0; i < numAutotasks; i++) {
                os_kill_thread(*(autotasks[i].id_ptr));
            }
            print_dbg("^C\n[CLI] All active background tasks forcefully killed.");
            cmd_idx = 0;
            continue;
        }
        
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cmd_idx > 0) {
                cmd_idx--;
                print_dbg("\b \b");
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            cmd[cmd_idx] = '\0'; 
            
            if (cmd_idx > 0) handleCommand(cmd);
            thread_sleep(100); // 100ms delay to ensure command processing before next prompt
            
            cmd_idx = 0;
            print_dbg("\n[CLI] > ");
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_idx < 127) {
                cmd[cmd_idx++] = c;
                print_dbg("%c", c);
            }
        }
    }
}
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
    char cmd_buffer[128];
    int cmd_buffer_idx = 0;
    
    print_dbg("\n[CLI-Thread] > ");

    while(1) {
        char c;
        while(1) {
            /* Violently clear UART hardware overrun/framing flags */
            if (LPUART1->STAT & (0xF << 16)) {
                LPUART1->STAT |= (0xF << 16); 
            }
            c = lpuartGetCharNonBlocking(LPUART1);
            if (c != '\0') break;
        }
        
        if (c == ASCII_CTRL_C) {
            // LOOP THROUGH AUTO-REGISTRY AND KILL EVERYTHING
            for (int i = 0; i < numAutotasks; i++) {
                os_kill_thread(*(autotasks[i].id_ptr));
            }
            print_dbg("^C\n[CLI-Thread] All active background tasks forcefully killed.\n");
            print_dbg("\n[CLI-Thread] > ");
            cmd_buffer_idx = 0;
            continue;
        }
        
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cmd_buffer_idx > 0) {
                cmd_buffer_idx--;
                print_dbg("\b \b");
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            cmd_buffer[cmd_buffer_idx] = '\0';
            
            print_dbg("\n");
            if (cmd_buffer_idx > 0) handleCommand(cmd_buffer);
            os_yield(); // Yield to allow any background tasks to print their output before we print the next prompt
            
            if(cmd_buffer_idx>0) print_dbg("\n[CLI-Thread] > ");
            else print_dbg("[CLI-Thread] > ");
            cmd_buffer_idx = 0;
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_buffer_idx < 127) {
                cmd_buffer[cmd_buffer_idx++] = c;
                print_dbg("%c", c);
            }
        }
    }
}
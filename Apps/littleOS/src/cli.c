#include <stdint.h>
#include "LPUART.h"
#include "include/cli.h"
#include "include/cli_utility.h"
#include "include/string.h"
#include "include/stdio.h"
#include "include/multitasking.h"
#include "include/autotasks.h"

static char cmd_history[CLI_HISTORY_SIZE][CMD_BUFFER_SIZE];
static int cmd_history_head = 0;
static int cmd_history_count = 0;

void cli_thread(void* arg) {
    char cmd_buffer[CMD_BUFFER_SIZE];
    char temp_buffer[CMD_BUFFER_SIZE]; // Saves current typing when browsing cmd_history
    
    int cmd_buffer_idx = 0;
    int cursorPos = 0;
    int historyView_idx = -1; // -1 means we are on the current typing line
    
    int escSeqState = 0; // for ESCAPE SEQUENCE STATE MACHINE (for arrow keys)

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
        
        // ESCAPE SEQUENCE STATE MACHINE (ARROW KEYS)
        if (escSeqState == 0 && c == 0x1B) { escSeqState = 1; continue; } // ESC
        if (escSeqState == 1 && c == 0x5B) { escSeqState = 2; continue; } // [
        if (escSeqState == 2) {
            if (c == 'A') { // UP ARROW
                if (cmd_history_count > 0) {
                    if (historyView_idx == -1) {
                        // Starting to browse: save what we were currently typing
                        cmd_buffer[cmd_buffer_idx] = '\0';
                        strcpy(temp_buffer, cmd_buffer);
                        historyView_idx = (cmd_history_head - 1 + CLI_HISTORY_SIZE) % CLI_HISTORY_SIZE;
                    } else {
                        int oldest_idx = (cmd_history_head - cmd_history_count + CLI_HISTORY_SIZE) % CLI_HISTORY_SIZE;
                        
                        // Only go backward if we haven't hit the "wall" yet
                        if (historyView_idx != oldest_idx) {
                            historyView_idx = (historyView_idx - 1 + CLI_HISTORY_SIZE) % CLI_HISTORY_SIZE;
                        }
                    }
                    // Load and redraw
                    strcpy(cmd_buffer, cmd_history[historyView_idx]);
                    cmd_buffer_idx = strlen(cmd_buffer);
                    cursorPos = cmd_buffer_idx;
                    print_dbg("\r\033[2K[CLI-Thread] > %s", cmd_buffer); // \033[2K clears the entire line
                }
            }
            else if (c == 'B') { // DOWN ARROW
                if (historyView_idx != -1) {
                    int latest_idx = (cmd_history_head - 1 + CLI_HISTORY_SIZE) % CLI_HISTORY_SIZE;
                    if (historyView_idx == latest_idx) {
                        // Reached the bottom, restore the temp typing buffer
                        historyView_idx = -1;
                        strcpy(cmd_buffer, temp_buffer);
                    } else {
                        historyView_idx = (historyView_idx + 1) % CLI_HISTORY_SIZE;
                        strcpy(cmd_buffer, cmd_history[historyView_idx]);
                    }
                    // Load and redraw
                    cmd_buffer_idx = strlen(cmd_buffer);
                    cursorPos = cmd_buffer_idx;
                    print_dbg("\r\033[2K[CLI-Thread] > %s", cmd_buffer);
                }
            }
            else if (c == 'C') { // RIGHT ARROW
                if (cursorPos < cmd_buffer_idx) {
                    cursorPos++;
                    print_dbg("\033[C"); // Move terminal cursor right
                }
            }
            else if (c == 'D') { // LEFT ARROW
                if (cursorPos > 0) {
                    cursorPos--;
                    print_dbg("\033[D"); // Move terminal cursor left
                }
            }
            
            escSeqState = 0;
            continue;
        }
        if (escSeqState != 0) escSeqState = 0; // Abort invalid escape sequence

        // If not arrow keys ...
        
        if (c == ASCII_CTRL_C) {
            for (int i = 0; i < numAutotasks; i++) {
                thread_kill(*(autotasks[i].id_ptr));
            }
            print_dbg("^C\n[CLI-Thread] All active background tasks forcefully killed.\n");
            print_dbg("\n[CLI-Thread] > ");
            
            cmd_buffer_idx = 0;
            cursorPos = 0;
            historyView_idx = -1;
            continue;
        }
        
        if (c == ASCII_BACKSPACE || c == ASCII_DEL) {
            if (cursorPos > 0) {
                // Shift buffer left
                for (int i = cursorPos; i < cmd_buffer_idx; i++) {
                    cmd_buffer[i - 1] = cmd_buffer[i];
                }
                cmd_buffer_idx--;
                cursorPos--;
                cmd_buffer[cmd_buffer_idx] = '\0';
                
                // Redraw entire line and fix cursor position
                print_dbg("\r\033[2K[CLI-Thread] > %s", cmd_buffer);
                for(int i = cmd_buffer_idx; i > cursorPos; i--) print_dbg("\b");
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            cmd_buffer[cmd_buffer_idx] = '\0';
            print_dbg("\n");
            
            if (cmd_buffer_idx > 0) {
                // Save to cmd_history (only if it's not an exact duplicate of the immediate last command)
                int last_idx = (cmd_history_head - 1 + CLI_HISTORY_SIZE) % CLI_HISTORY_SIZE;
                if (cmd_history_count == 0 || strcmp(cmd_history[last_idx], cmd_buffer) != 0) {
                    strcpy(cmd_history[cmd_history_head], cmd_buffer);
                    cmd_history_head = (cmd_history_head + 1) % CLI_HISTORY_SIZE;
                    if (cmd_history_count < CLI_HISTORY_SIZE) cmd_history_count++;
                }
                
                handleCommand(cmd_buffer);
            }
            else if(cmd_buffer_idx==0) print_dbg("[CLI-Thread] > ");
            
            // Reset states for the next command
            cmd_buffer_idx = 0;
            cursorPos = 0;
            historyView_idx = -1;
        } 
        else if (c >= 32 && c <= 126) {
            if (cmd_buffer_idx < CMD_BUFFER_SIZE - 1) {
                // Shift buffer right to make room (if cursor is not at the end)
                for (int i = cmd_buffer_idx; i > cursorPos; i--) {
                    cmd_buffer[i] = cmd_buffer[i - 1];
                }
                cmd_buffer[cursorPos] = c;
                cmd_buffer_idx++;
                cursorPos++;
                cmd_buffer[cmd_buffer_idx] = '\0';
                
                // Redraw entire line and fix cursor position
                print_dbg("\r\033[2K[CLI-Thread] > %s", cmd_buffer);
                for(int i = cmd_buffer_idx; i > cursorPos; i--) print_dbg("\b");
            }
        }
    }
}
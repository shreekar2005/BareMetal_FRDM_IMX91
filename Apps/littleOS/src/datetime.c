#include "../include/multitasking.h"
#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/esp8266.h"
#include "../include/datetime.h"

// Simulated Hardware RTC Registers
static int sys_hour = 0, sys_min = 0, sys_sec = 0;
static int sys_day = 1, sys_month = 1, sys_year = 2026;

void datetime_ticker_thread(void* arg) {
    while(1) {
        thread_sleep(1000); 
        
        sys_sec++;
        if (sys_sec >= 60) {
            sys_sec = 0;
            sys_min++;
            if (sys_min >= 60) {
                sys_min = 0;
                sys_hour++;
                if (sys_hour >= 24) {
                    sys_hour = 0;
                    sys_day++;
                    
                    // Simple month rollover (Assumes 30 days for simplicity in littleOS)
                    int days_in_month = 31;
                    if (sys_month == 2) days_in_month = 28;
                    else if (sys_month == 4 || sys_month == 6 || sys_month == 9 || sys_month == 11) days_in_month = 30;
                    
                    if (sys_day > days_in_month) {
                        sys_day = 1;
                        sys_month++;
                        if (sys_month > 12) {
                            sys_month = 1;
                            sys_year++;
                        }
                    }
                }
            }
        }
    }
}

// NOW PARSING THE CMD STRING DIRECTLY
void datetime_handlecmd(const char* cmd) {
    char cmd_name[16] = {0};
    char subcmd[16] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};
    int ptr = 0, i = 0;

    // Parse the main command string ("datetime")
    while (cmd[ptr] == ' ') ptr++;
    while (cmd[ptr] != ' ' && cmd[ptr] != '\r' && cmd[ptr] != '\n' && cmd[ptr] != '\0' && i < 15) cmd_name[i++] = cmd[ptr++];
    cmd_name[i] = '\0';

    // Parse the subcommand (e.g., "show", "sync", "set")
    i = 0;
    while (cmd[ptr] == ' ') ptr++;
    while (cmd[ptr] != ' ' && cmd[ptr] != '\r' && cmd[ptr] != '\n' && cmd[ptr] != '\0' && i < 15) subcmd[i++] = cmd[ptr++];
    subcmd[i] = '\0';

    // Parse arg1 (e.g., "hh:mm:ss" or IP)
    i = 0;
    while (cmd[ptr] == ' ') ptr++;
    while (cmd[ptr] != ' ' && cmd[ptr] != '\r' && cmd[ptr] != '\n' && cmd[ptr] != '\0' && i < 31) arg1[i++] = cmd[ptr++];
    arg1[i] = '\0';

    // Parse arg2 (e.g., "dd:mm:yyyy" or Port)
    i = 0;
    while (cmd[ptr] == ' ') ptr++;
    while (cmd[ptr] != ' ' && cmd[ptr] != '\r' && cmd[ptr] != '\n' && cmd[ptr] != '\0' && i < 31) arg2[i++] = cmd[ptr++];
    arg2[i] = '\0';

    if (strcmp(subcmd, "show") == 0) {
        datetime_show();
    } else if (strcmp(subcmd, "set") == 0) {
        datetime_set(arg1, arg2);
    } else if (strcmp(subcmd, "sync") == 0) {
        datetime_sync(arg1, arg2);
    } else {
        print_dbg("[DATETIME-Thread] Invalid argument. Usage:\n");
        print_dbg("[DATETIME-Thread]  datetime show                                  (Prints current time)\n");
        print_dbg("[DATETIME-Thread]  datetime sync <server_ip> <port>               (Fetches real time via TCP)\n");
        print_dbg("[DATETIME-Thread]  datetime set  <hh:mm:ss> <dd:mm:yyyy>          (Manually update RTC)\n");
    }
}

void datetime_show(void) {
    print_dbg("[DATETIME-Thread] Current System Date/Time:\n");
    print_dbg("[DATETIME-Thread]      %02d:%02d:%02d  %02d/%02d/%04d\n", sys_hour, sys_min, sys_sec, sys_day, sys_month, sys_year);
}

void datetime_set(const char* arg1, const char* arg2) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        print_dbg("[DATETIME-Thread] Error: Requires <hh:mm:ss> <dd:mm:yyyy>\n");
        return;
    }
    
    // Very simple string parsing (assuming strict formatting)
    sys_hour  = (arg1[0]-'0')*10 + (arg1[1]-'0');
    sys_min   = (arg1[3]-'0')*10 + (arg1[4]-'0');
    sys_sec   = (arg1[6]-'0')*10 + (arg1[7]-'0');
    
    sys_day   = (arg2[0]-'0')*10 + (arg2[1]-'0');
    sys_month = (arg2[3]-'0')*10 + (arg2[4]-'0');
    sys_year  = (arg2[6]-'0')*1000 + (arg2[7]-'0')*100 + (arg2[8]-'0')*10 + (arg2[9]-'0');

    print_dbg("[DATETIME-Thread] Hardware clock updated successfully!\n");
}

void datetime_sync(const char* arg1, const char* arg2) {
    const char* target_ip = (arg1[0] != '\0') ? arg1 : "192.168.21.103"; 
    int target_port = (arg2[0] != '\0') ? atoi(arg2) : 5555;
    
    print_dbg("[DATETIME-Thread] Requesting time sync from %s:%d...\n", target_ip, target_port);
    
    esp_sendto_tcp_clients(target_ip, target_port, "GET_TIME\n");
    
    print_dbg("[DATETIME-Thread] Awaiting callback from server...\n");
}
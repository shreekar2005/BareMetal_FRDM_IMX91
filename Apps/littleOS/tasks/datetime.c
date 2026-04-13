// Task_Name : datetime
#include "../include/multitasking.h"
#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/esp8266.h"

extern volatile char print_buffer[128];

// Simulated Hardware RTC Registers (will replace with i.MX91 SNVS later)
static int sys_hour = 0, sys_min = 0, sys_sec = 0;
static int sys_day = 1, sys_month = 1, sys_year = 2026;

void datetime_thread(void* arg) {
    char cmd[16] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};
    int ptr = 0, i = 0;

    // Parse main command (e.g., show, sync, set)
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 15) cmd[i++] = print_buffer[ptr++];
    cmd[i] = '\0';

    // Parse arg1 (e.g., hh:mm:ss or IP)
    i = 0;
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 31) arg1[i++] = print_buffer[ptr++];
    arg1[i] = '\0';

    // Parse arg2 (e.g., dd:mm:yyyy or Port)
    i = 0;
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 31) arg2[i++] = print_buffer[ptr++];
    arg2[i] = '\0';


    if (strcmp(cmd, "show") == 0) {
        print_dbg("\r\n[RTC] Current System Date/Time:\r\n");
        print_dbg("      %02d:%02d:%02d  %02d/%02d/%04d\r\n", sys_hour, sys_min, sys_sec, sys_day, sys_month, sys_year);
    } 
    else if (strcmp(cmd, "set") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            print_dbg("[RTC] Error: Requires <hh:mm:ss> <dd:mm:yyyy>\r\n");
        } else {
            // Very simple string parsing (assuming strict formatting)
            sys_hour  = (arg1[0]-'0')*10 + (arg1[1]-'0');
            sys_min   = (arg1[3]-'0')*10 + (arg1[4]-'0');
            sys_sec   = (arg1[6]-'0')*10 + (arg1[7]-'0');
            
            sys_day   = (arg2[0]-'0')*10 + (arg2[1]-'0');
            sys_month = (arg2[3]-'0')*10 + (arg2[4]-'0');
            sys_year  = (arg2[6]-'0')*1000 + (arg2[7]-'0')*100 + (arg2[8]-'0')*10 + (arg2[9]-'0');

            print_dbg("[RTC] Hardware clock updated successfully!\r\n");
        }
    } 
    else if (strcmp(cmd, "sync") == 0) {
        // We need the IP of your server. If not provided, use a default.
        const char* target_ip = (arg1[0] != '\0') ? arg1 : "192.168.21.100"; // Put your server IP here!
        int target_port = (arg2[0] != '\0') ? atoi(arg2) : 5000;
        
        print_dbg("[RTC] Requesting time sync from %s:%d...\r\n", target_ip, target_port);
        
        os_stop_scheduling();
        esp_tcp_client_send(target_ip, target_port, "GET_TIME");
        os_start_scheduling();
        
        print_dbg("[RTC] Awaiting callback from server...\r\n");
    }
    else {
        // Help Menu
        print_dbg("\r\n[RTC] Invalid argument. Usage:\r\n");
        print_dbg("  datetime show                                  (Prints current time)\r\n");
        print_dbg("  datetime sync <server_ip> <port>               (Fetches real time via TCP)\r\n");
        print_dbg("  datetime set  <hh:mm:ss> <dd:mm:yyyy>          (Manually update RTC)\r\n");
    }

    print_dbg("\n> ");
}
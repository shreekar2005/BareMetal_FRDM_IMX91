// Task_Name : ESP Commands

#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/esp8266.h"
#include "include/string.h"

extern volatile char print_buffer[128]; /**< buffer defined in cli.c */

void esp_thread(void* arg) {

    // Buffers to hold our parsed words
    char cmd[16] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};
    int ptr = 0;
    int i = 0;

    // main command (ap-mode, sta-mode, tcp-server, echo, stat)
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 15) {
        cmd[i++] = print_buffer[ptr++];
    }
    cmd[i] = '\0';

    int remainder_ptr = ptr;
    while (print_buffer[remainder_ptr] == ' ') remainder_ptr++; 

    // arg1 (SSID or Port)
    i = 0;
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 31) {
        arg1[i++] = print_buffer[ptr++];
    }
    arg1[i] = '\0';

    // arg2 (Password)
    i = 0;
    while (print_buffer[ptr] == ' ') ptr++;
    while (print_buffer[ptr] != ' ' && print_buffer[ptr] != '\0' && i < 31) {
        arg2[i++] = print_buffer[ptr++];
    }
    arg2[i] = '\0';

    if (strcmp(cmd, "ap-mode") == 0) {
        const char* ssid = (arg1[0] != '\0') ? arg1 : "littleOS";
        const char* pass = (arg2[0] != '\0') ? arg2 : "littleos";
        
        int pass_len = 0;
        while(pass[pass_len] != '\0') pass_len++;
        
        if (pass_len < 8) {
            print_dbg("[ESP] Error: WPA2 passwords MUST be at least 8 characters long!\r\n");
        } else {
            os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
            init_esp_as_access_point(ssid, pass);
            os_start_scheduling(); // Resume normal OS multitasking
        }
    }

    else if (strcmp(cmd, "sta-mode") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            print_dbg("[ESP] Error: 'sta-mode' requires both <ssid_name> and <ssid_password>.\r\n");
        } else {
            os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
            init_esp_as_station(arg1, arg2);
            os_start_scheduling(); // Resume normal OS multitasking
        }
    } 
    else if (strcmp(cmd, "tcp-server") == 0) {
        // Use default port 8080 if arg1 is empty
        int port = (arg1[0] != '\0') ? atoi(arg1) : 8080;
        os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
        start_esp_tcp_server(port);
        os_start_scheduling(); // Resume normal OS multitasking
    } 
    else if (strcmp(cmd, "echo") == 0) {
        // Send everything typed after "echo " over Wi-Fi
        if (print_buffer[remainder_ptr] != '\0') {
            os_stop_scheduling(); // Lock scheduling
            print_esp("%s\n", (const char*)&print_buffer[remainder_ptr]);
            os_start_scheduling(); // Resume scheduling
            print_dbg("[ESP] Echo sent: %s\r\n", &print_buffer[remainder_ptr]);
        } else {
            print_dbg("[ESP] Error: Nothing to echo. Usage: esp echo <message>\r\n");
        }
    }
    else if (strcmp(cmd, "status") == 0) {
        os_stop_scheduling(); // Lock scheduling
        print_esp_status();     // Call hardware driver to query module
        os_start_scheduling(); // Resume scheduling
    }

    else {
        // Print updated help menu
        print_dbg("[ESP] Invalid argument.\r\n");
        print_dbg("Usage:\r\n");
        print_dbg("  sta-mode   <ssid_name> <ssid_password>  (Both required)\r\n");
        print_dbg("  ap-mode    [ssid_name] [ssid_password]  (Default: littleOS / littleos)\r\n");
        print_dbg("  tcp-server [port_number]                (Default: 8080)\r\n");
        print_dbg("  echo       <message>                    (Sends text to TCP clients)\r\n");
        print_dbg("  status                                  (Shows current mode, IP, MAC)\r\n");
    }
    
    print_dbg("\n> ");
}
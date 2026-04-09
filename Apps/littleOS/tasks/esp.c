// Task_Name : ESP Commands

#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/esp8266.h"
#include "include/string.h"

extern volatile char print_buffer[128];

void esp_thread(void* arg) {

    // Buffers to hold our parsed words
    char cmd[16] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};
    int ptr = 0;
    int i = 0;

    // main command (ap-mode, sta-mode, tcp-server, echo)
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

    if (my_strcmp(cmd, "ap-mode") == 0) {
        const char* ssid = (arg1[0] != '\0') ? arg1 : "littleOS";
        const char* pass = (arg2[0] != '\0') ? arg2 : "littleos";
        
        int pass_len = 0;
        while(pass[pass_len] != '\0') pass_len++;
        
        if (pass_len < 8) {
            printdbg("[ESP] Error: WPA2 passwords MUST be at least 8 characters long!\r\n");
        } else {
            os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
            init_esp_access_point(ssid, pass);
            os_start_scheduling(); // Resume normal OS multitasking
        }
    }

    else if (my_strcmp(cmd, "sta-mode") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            printdbg("[ESP] Error: 'sta-mode' requires both <ssid_name> and <ssid_password>.\r\n");
        } else {
            os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
            init_esp_station(arg1, arg2);
            os_start_scheduling(); // Resume normal OS multitasking
        }
    } 
    else if (my_strcmp(cmd, "tcp-server") == 0) {
        // Use default port 8080 if arg1 is empty
        int port = (arg1[0] != '\0') ? my_atoi(arg1) : 8080;
        os_stop_scheduling(); // Ensure no other tasks interfere with Wi-Fi init
        init_esp_tcp_server(port);
        os_start_scheduling(); // Resume normal OS multitasking
    } 
    else if (my_strcmp(cmd, "echo") == 0) {
        // Send everything typed after "echo " over Wi-Fi
        if (print_buffer[remainder_ptr] != '\0') {
            printesp("%s\n", (const char*)&print_buffer[remainder_ptr]);
            printdbg("[ESP] Echo sent: %s\r\n", &print_buffer[remainder_ptr]);
        } else {
            printdbg("[ESP] Error: Nothing to echo. Usage: esp echo <message>\r\n");
        }
    }
    else {
        // Print updated help menu
        printdbg("[ESP] Invalid argument.\r\n");
        printdbg("Usage:\r\n");
        printdbg("  ap-mode    [ssid_name] [ssid_password]  (Default: littleOS / littleos)\r\n");
        printdbg("  sta-mode   <ssid_name> <ssid_password>  (Both required)\r\n");
        printdbg("  tcp-server [port_number]                (Default: 8080)\r\n");
        printdbg("  echo       <message>                    (Sends text to TCP clients)\r\n");
    }
    
    printdbg("\n> ");
}
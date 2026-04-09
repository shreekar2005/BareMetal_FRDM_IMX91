#include "include/esp8266.h"
#include "include/stdio.h"

/* Actively listens to the ESP8266 and prints exactly as characters arrive */
static void print_esp_response(volatile uint32_t timeout_cycles) {
    char c;
    printdbg("[ESP8266] ");
    
    while(timeout_cycles--) {
        c = lpuart_getchar_nonblocking(LPUART4);
        if (c != '\0') {
            /* If we got a character, print it to the debug screen immediately */
            printdbg("%c", c);
        }
        __asm__ volatile("nop"); 
    }
    
    printdbg("\r\n");
}

void init_esp_tcp_server(int port) {
    printdbg("\r\n[Wi-Fi] Starting TCP Server on port %d...\r\n", port);

    /* Enable Multiple Connections (Required for Server mode) */
    printesp("AT+CIPMUX=1\r\n");
    print_esp_response(5000000);

    /* Start the Server: AT+CIPSERVER=<mode>,<port> 
     * Mode 1 = Create server */
    printesp("AT+CIPSERVER=1,%d\r\n", port);
    print_esp_response(5000000);
    
    /* Check the ESP's IP address so you know where to connect */
    printesp("AT+CIFSR\r\n");
    print_esp_response(5000000);

    printdbg("[Wi-Fi] TCP Server is running and listening!\r\n");
}

void init_esp_access_point(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Access Point (AP) Mode...\r\n");

    /* Test communication */
    printesp("AT\r\n");
    print_esp_response(5000000); 

    /* Set Wi-Fi Mode to 2 (SoftAP) */
    printesp("AT+CWMODE=2\r\n");
    print_esp_response(5000000);

    /* Configure the AP: AT+CWSAP="ssid","pwd",channel,encryption 
     * Encryption 3 = WPA2_PSK */
    printesp("AT+CWSAP=\"%s\",\"%s\",1,3\r\n", ssid, password);
    print_esp_response(15000000); // AP creation takes slightly longer
    
    printdbg("[Wi-Fi] Access Point '%s' is now broadcasting.\r\n", ssid);
}


void init_esp_station(const char* ssid, const char* password) {
    printdbg("\r\n[Wi-Fi] Initializing ESP8266 in Station Mode...\r\n");

    /* Test communication */
    printesp("AT\r\n");
    print_esp_response(5000000);

    /* Set Wi-Fi Mode to 1 (Station) */
    printesp("AT+CWMODE=1\r\n");
    print_esp_response(5000000);

    /* Connect to the Access Point: AT+CWJAP="ssid","pwd" */
    printesp("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    
    printdbg("[Wi-Fi] Attempting connection... (This takes a few seconds)\r\n");
    print_esp_response(40000000); // Wi-Fi handshake takes several seconds
    
    printdbg("[Wi-Fi] Station connection sequence complete.\r\n");
}
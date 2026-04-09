// Task_Name : Echo on ESP8266

#include "include/esp8266.h"

extern volatile char print_buffer[128]; /**< buffer defined in cli.c */

void echoesp_thread(void* arg) {
    printesp("%s", (const char*)print_buffer);
}
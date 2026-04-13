#ifndef STATUS_H
#define STATUS_H

#include "include/multitasking.h"
#include "SYS_CTR.h"
#include "include/stdio.h"
#include <stdbool.h>

typedef struct {
    const char* name;
    const char* current_state;
    uint64_t priority, deadline, period, targetExecs, doneExecs, lastTAT;
} threadStatParams;

typedef struct {
    bool reachable;
    int op_mode;
    char router_ssid[33];   // Max 32 chars for WPA2 + null terminator
    char router_mac[18];    // 17 chars for XX:XX:XX:XX:XX:XX + null terminator
    char esp_ip[16];        // 15 chars for 255.255.255.255 + null terminator
    char esp_mac[18];       // 17 chars + null terminator
} espStatParams;

// Task Entrypoint
void status_thread(void* arg);

void getTasksInfo(threadStatParams allThreads[]);
void getEspInfo(espStatParams* espInstance);

/**
 * @brief Listens to the ESP UART and stores the response in a buffer
 * @param buffer The array to store the raw string
 * @param max_len Maximum capacity of the buffer
 * @param timeout_sec Timeout in seconds
 * @return true if "OK" was received, false if "ERROR", "FAIL", or timeout
 */
bool get_raw_esp_response(char* buffer, int max_len, uint32_t timeout_sec);

/**
 * @brief Parses raw AT command output from the ESP8266 into a structured format
 * @param raw_buffer The full string buffer containing the ESP's response
 */
void parse_esp_response(const char* raw_buffer);

/**
 * @brief Constructs the final payload string from the gathered statistics
 */
void build_stats_string(char* buffer, threadStatParams allThreads[], espStatParams espInstance);

#endif // STATUS_H
#ifndef STATISTICS_H
#define STATISTICS_H

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


/**
 * @brief Gathers information about all threads and fills the provided array of threadStatParams
 * @param allThreads Array of threadStatParams to fill with the gathered information (size should
 */
void getTasksInfo(threadStatParams allThreads[]);

/**
 * @brief Gathers information about the ESP8266 module by sending AT commands and parsing responses
 * @param espInstance Pointer to an espStatParams struct to fill with the gathered information
 */
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
 * @brief Constructs the final payload string from the gathered statistics
 * @param buffer The array to store the final string
 * @param allThreads Array of threadStatParams for each thread
 * @param espInstance The espStatParams instance containing ESP info
 */
void build_stats_string(char* buffer, threadStatParams allThreads[], espStatParams espInstance);


#endif // STATISTICS_H
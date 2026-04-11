# include "include/multitasking.h"
# include "include/statistics.h"
# include "include/string.h"

void statistics_thread(void* arg)
{
    // pollPeriod = *(int*)(arg);
    // pollPeriod = pollPeriod<3?3:pollPeriod;
    pollPeriod=5;
    while(1)
    {
        threadStatParams allThreads[numThreads];
        getTasksInfo(allThreads);

        espStatParams espInstance;
        espInstance.reachable=false;
        espInstance.op_mode=-1;
        espInstance.esp_ip[0]='\0';
        espInstance.esp_mac[0]='\0';
        espInstance.router_ssid[0]='\0';
        espInstance.router_mac[0]='\0';

        os_stop_scheduling();
        getEspInfo(&espInstance);
        os_start_scheduling();

        print_stats(allThreads, espInstance);

        thread_sleep(pollPeriod*1000);
    }
}

void getTasksInfo(threadStatParams allThreads[])
{
    // thread info
    for (int i = 0; i < numThreads; i++) {
        allThreads[i].name = threads[i].name;
        allThreads[i].current_state = get_thread_state_name(threads[i].currentState);
        allThreads[i].deadline = threads[i].deadlineOffset_ms;
        allThreads[i].doneExecs = threads[i].executionsDone;
        allThreads[i].targetExecs = threads[i].executionsTarget; //can be -1
        allThreads[i].priority = threads[i].priority;
        allThreads[i].period = threads[i].period_ms; // can be -1
        allThreads[i].lastTAT = threads[i].lastTurnaroundTime_ms;
    }

    schedAlgo = currentSchedAlgo;
}

void getEspInfo(espStatParams* espInstance)
{
    send_to_esp("AT\r\n"); //reachability check
    if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 3))
    {
        espInstance->reachable = true;
    }

    send_to_esp("AT+CWMODE?\r\n"); //op_mode check
    if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 3))
    {
        const char* mode_ptr = strstr(raw_esp_response_buffer, "+CWMODE:");
        if (mode_ptr)
        {
            mode_ptr += 8; // Move pointer past the "+CWMODE:" prefix
        
            // Extract the single digit mode directly to avoid atoi edge cases
            if (*mode_ptr >= '1' && *mode_ptr <= '3') {
                espInstance->op_mode = *mode_ptr - '0';
            }
        }
    }
    

    if (espInstance->op_mode == 1) //station mode
    {
        send_to_esp("AT+CWJAP?\r\n"); //router info
        if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 5))
        {
            const char* jap_ptr = strstr(raw_esp_response_buffer, "+CWJAP:\"");
            if (jap_ptr)
            {
                jap_ptr += 8; // Move pointer past '+CWJAP:"'
        
                // Extract SSID until the next quote
                int i = 0;
                while (*jap_ptr != '"' && *jap_ptr != '\0' && i < 32) {
                    espInstance->router_ssid[i++] = *jap_ptr++;
                }
                espInstance->router_ssid[i] = '\0';

                // Find the start of the MAC address (located after the next ',"')
                jap_ptr = strstr(jap_ptr, ",\"");
                if (jap_ptr) {
                    jap_ptr += 2; // Move past ',"'
                    i = 0;
                    while (*jap_ptr != '"' && *jap_ptr != '\0' && i < 17) {
                        espInstance->router_mac[i++] = *jap_ptr++;
                    }
                    espInstance->router_mac[i] = '\0';
                }
            }   
        }
        
        send_to_esp("AT+CIFSR\r\n"); //esp ip and mac
        if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 3))
        {
            const char* ip_ptr = strstr(raw_esp_response_buffer, "+CIFSR:STAIP,\"");
            if (!ip_ptr)
            {
                ip_ptr = strstr(raw_esp_response_buffer, "+CIFSR:APIP,\"");
                if (ip_ptr) ip_ptr += 13; // Move past '+CIFSR:APIP,"'
            }
            else
            {
                ip_ptr += 14; // Move past '+CIFSR:STAIP,"'
            }

            if (ip_ptr)
            {
                int i = 0;
                while (*ip_ptr != '"' && *ip_ptr != '\0' && i < 15) {
                    espInstance->esp_ip[i++] = *ip_ptr++;
                }
                espInstance->esp_ip[i] = '\0';
            }

            // 6. Extract ESP MAC (AT+CIFSR)
            // Could be STAMAC or APMAC
            const char* mac_ptr = strstr(raw_esp_response_buffer, "+CIFSR:STAMAC,\"");
            if (!mac_ptr) {
                mac_ptr = strstr(raw_esp_response_buffer, "+CIFSR:APMAC,\"");
                if (mac_ptr) mac_ptr += 14; // Move past '+CIFSR:APMAC,"'
            } else {
                mac_ptr += 15; // Move past '+CIFSR:STAMAC,"'
            }

            if (mac_ptr)
            {
                int i = 0;
                while (*mac_ptr != '"' && *mac_ptr != '\0' && i < 17) {
                    espInstance->esp_mac[i++] = *mac_ptr++;
                }
                espInstance->esp_mac[i] = '\0';
            }
        }
    }
}

bool get_raw_esp_response(char* buffer, int max_len, uint32_t timeout_sec) {
    char c;
    char prev = 0;
    int idx = 0;
    bool success = false;
    bool timed_out = true;

    // Initialize buffer as empty string
    if (max_len > 0) buffer[0] = '\0';

    uint64_t targetClockTick = sysctrGetTicks() + timeout_sec * sysctrGetFreq(); 

    while (sysctrGetTicks() < targetClockTick) {
        /* Clear Overrun errors just in case */
        if (LPUART4->STAT & (0xF << 16)) {
            LPUART4->STAT |= (0xF << 16); 
        }

        c = lpuartGetCharNonBlocking(LPUART4);
        
        if (c != '\0') {
            // Save the character to our buffer, leaving room for null terminator
            if (idx < max_len - 1) {
                buffer[idx++] = c;
                buffer[idx] = '\0';
            }

            // Check if ESP replied "OK"
            if (prev == 'O' && c == 'K') {
                // Read the final '\r' and '\n' to clear the pipe before exiting
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = lpuartGetCharNonBlocking(LPUART4);
                    if (flush_c != '\0' && idx < max_len - 1) {
                        buffer[idx++] = flush_c;
                        buffer[idx] = '\0';
                    }
                    if (flush_c == '\n') break;
                }
                success = true;
                timed_out = false;
                break;
            }
            
            // Check if ESP replied ERR"OR"
            if (prev == 'O' && c == 'R') {
                success = false;
                timed_out = false;
                break;
            }

            // Check if ESP replied FA"IL"
            if (prev == 'I' && c == 'L') {
                success = false;
                timed_out = false;
                break;
            }

            prev = c;
        }
        __asm__ volatile("nop"); 
    }
    
    return success;
}

void print_stats(threadStatParams allThreads[], espStatParams espInstance)
{
    print_dbg("Current scheduling algorithm used = %-16s\n", schedAlgo);
    for(int i=0;i<numThreads;i++)
    {
        print_dbg("\nThread %d\n", (i+1));
        print_dbg("\nName of thread = %-16s\nCurrent state of thread = %-16s\nPriority of thread = %d\nDeadline of thread = %d\nPeriod of thread = %d\nNo. of target executions of thread = %d\nNo. of executions done already = %6d\nLast turn-around-time of thread = %d\n",
        allThreads[i].name, allThreads[i].current_state, allThreads[i].priority, allThreads[i].deadline, allThreads[i].period, allThreads[i].targetExecs, allThreads[i].doneExecs, allThreads[i].lastTAT);
    }

    print_dbg("ESP status:\n");
    bool reachable;
    int op_mode;
    char router_ssid[33];   // Max 32 chars for WPA2 + null terminator
    char router_mac[18];    // 17 chars for XX:XX:XX:XX:XX:XX + null terminator
    char esp_ip[16];        // 15 chars for 255.255.255.255 + null terminator
    char esp_mac[18];
    if(espInstance.reachable == true) print_dbg("Reachable = true\n");
    else print_dbg("Reachable = false\n");
    print_dbg("Operating mode of ESP = %d\n", op_mode);
    if(router_ssid != '\0') print_dbg("Router ssid = %-16s\n", router_ssid);
    else print_dbg("Router ssid = Not applicable\n");
    if(router_mac != '\0') print_dbg("Router mac = %-16s\n", router_mac);
    else print_dbg("Router mac = Not applicable\n");
    if(esp_ip!='\0') print_dbg("ESP IP = %-16s\n", esp_ip);
    else print_dbg("ESP IP = Not applicable\n");
    if(esp_mac!='\0') print_dbg("ESP mac = %-16s\n", esp_mac);
    else print_dbg("ESP mac = Not applicable\n");
}
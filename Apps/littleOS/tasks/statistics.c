// Task_Name : Show/Send Stats

#include <stddef.h>
#include "include/multitasking.h"
#include "tasks_include/statistics.h"
#include "include/string.h"
#include "include/stdio.h"
#include "include/esp8266.h"

/* STACK OVERFLOW PREVENTION : Moving massive arrays from the 4KB RTOS Thread Stack to static BSS memory. */
static char payload_buffer[2048]={0}; 
static char raw_esp_response_buffer[512]={0}; 
static threadStatParams allThreads[MAX_THREADS];


static void append_int(char* buf, int val) {
    char temp[16];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        if (val < 0) {
            strcat(buf, "-");
            val = -val;
        }
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
    }
    int len = strlen(buf);
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
}

void getTasksInfo(threadStatParams allThreads[]);
void getEspInfo(espStatParams* espInstance);
bool get_raw_esp_response(char* buffer, int max_len, uint32_t timeout_sec);
void build_stats_string(char* buffer, threadStatParams allThreads[], espStatParams espInstance);

void statistics_thread(void* arg){
    print_dbg("\n");
    char* cmd_string = (char*)arg;

    char action[16] = {0};
    char arg_ip[32] = {0};
    char arg_port[32] = {0};
    int ptr = 0, i = 0;

    if (cmd_string != NULL) {
        while (cmd_string[ptr] == ' ') ptr++;
        while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 15) action[i++] = cmd_string[ptr++];
        action[i] = '\0';
    }

    if (action[0] == '\0' || strcmp(action, "help") == 0 || strcmp(action, "?") == 0 || strcmp(action, "--help") == 0) {
        print_dbg("[STATISTICS-Thread] Usage:\n");
        print_dbg("[STATISTICS-Thread]   statistics show                                  (prints statistics to debug console)\n");
        print_dbg("[STATISTICS-Thread]   statistics sendto <target_ip> <target_port>      (Sends to custom IP and Port)\n");
        return; 
    }

    if (strcmp(action, "sendto") == 0) {
        i = 0;
        while (cmd_string[ptr] == ' ') ptr++;
        while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 31) arg_ip[i++] = cmd_string[ptr++];
        arg_ip[i] = '\0';

        i = 0;
        while (cmd_string[ptr] == ' ') ptr++;
        while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 31) arg_port[i++] = cmd_string[ptr++];
        arg_port[i] = '\0';

        if (arg_ip[0] != '\0') {
            bool has_dot = false;
            for(int j = 0; arg_ip[j] != '\0'; j++) {
                if(arg_ip[j] == '.') has_dot = true;
            }
            if (!has_dot && strcmp(arg_ip, "localhost") != 0) {
                print_dbg("[STATISTICS-Thread] Invalid IP argument: %s\n", arg_ip);
                print_dbg("[STATISTICS-Thread] Type 'statistics help' for usage.");
                return;
            }
        }
    } else if (strcmp(action, "show") != 0) {
        print_dbg("[STATISTICS-Thread] Unknown command: %s\n", action);
        print_dbg("[STATISTICS-Thread] Type 'statistics help' for usage.");
        return;
    }

    const char* target_ip = (arg_ip[0] != '\0') ? arg_ip : "192.168.4.2";
    int target_port = (arg_port[0] != '\0') ? atoi(arg_port) : 5555;

    /* Using the global static allThreads array to prevent VLA stack overflow */
    getTasksInfo(allThreads);

    const char* schedAlgo;
    switch(currentSchedAlgo)
    {
        case SCHED_RR: schedAlgo = "Round Robin"; break;
        case SCHED_PRIORITY: schedAlgo = "Priority Scheduling"; break;
        case SCHED_EDF: schedAlgo = "Earliest Deadline First"; break;
        default: schedAlgo = "Unknown";
    }

    espStatParams espInstance;
    
    char* p = (char*)&espInstance;
    for(unsigned int clr = 0; clr < sizeof(espStatParams); clr++) {
        p[clr] = '\0';
    }

    espInstance.reachable=false;
    espInstance.op_mode=-1;

    getEspInfo(&espInstance);

    uint64_t ticks = sysctrGetTicks();
    uint64_t freq = sysctrGetFreq();
    uint64_t uptime_sec = ticks/freq;
    int sec = uptime_sec % 60;
    uptime_sec /= 60;
    int min = uptime_sec % 60;
    uptime_sec /= 60;
    int hr = uptime_sec;

    if (strcmp(action, "show") == 0) {
        /* DIRECT TERMINAL PRINTING (TABLE FORMAT) */
        print_dbg("[STATISTICS-Thread] UPTIME:\n");
        print_dbg("[STATISTICS-Thread]  %d hrs : %d min : %d sec\n", hr, min, sec);
        print_dbg("[STATISTICS-Thread]\n");
        print_dbg("[STATISTICS-Thread] SCHEDULER:\n");
        print_dbg("[STATISTICS-Thread]  Algorithm: %s\n", schedAlgo);
        print_dbg("[STATISTICS-Thread]\n");
        print_dbg("[STATISTICS-Thread] THREADS:\n");
        print_dbg("[STATISTICS-Thread]  %-3s | %-16s | %-5s | %-4s | %-5s | %-5s | %-4s | %-5s | %-7s\n", 
                  "ID", "Name", "State", "Pri", "Dead", "Per", "Targ", "Done", "TAT(ms)");
        print_dbg("[STATISTICS-Thread]  --------------------------------------------------------------------------------\n");
        
        for (int j = 0; j < numThreads; j++) {
            print_dbg("[STATISTICS-Thread]  %-3d | %-16s | %-5s | %-4d | %-5d | %-5d | %-4d | %-5d | %-7d\n",
                      j + 1, 
                      allThreads[j].name, 
                      allThreads[j].current_state,
                      (int)allThreads[j].priority, 
                      (int)allThreads[j].deadline, 
                      (int)allThreads[j].period,
                      (int)allThreads[j].targetExecs, 
                      (int)allThreads[j].doneExecs, 
                      (int)allThreads[j].lastTAT);
        }

        print_dbg("[STATISTICS-Thread]\n");
        print_dbg("[STATISTICS-Thread] ESP STATUS:\n");
        print_dbg("[STATISTICS-Thread]  Reachable: %s\n", espInstance.reachable ? "true" : "false");
        print_dbg("[STATISTICS-Thread]  Op Mode: %d\n", espInstance.op_mode);
        print_dbg("[STATISTICS-Thread]  Router SSID: %s\n", espInstance.router_ssid[0] != '\0' ? espInstance.router_ssid : "N/A");
        print_dbg("[STATISTICS-Thread]  Router MAC: %s\n", espInstance.router_mac[0] != '\0' ? espInstance.router_mac : "N/A");
        print_dbg("[STATISTICS-Thread]  ESP IP: %s\n", espInstance.esp_ip[0] != '\0' ? espInstance.esp_ip : "N/A");
        print_dbg("[STATISTICS-Thread]  ESP MAC: %s\n", espInstance.esp_mac[0] != '\0' ? espInstance.esp_mac : "N/A");
    }
    else if (strcmp(action, "sendto") == 0) {
        /* COMPACT CSV PAYLOAD FOR WEB APP */
        payload_buffer[0] = '\0';
        
        strcat(payload_buffer, "STATUS:\n");
        
        strcat(payload_buffer, "U,"); append_int(payload_buffer, hr); strcat(payload_buffer, ",");
        append_int(payload_buffer, min); strcat(payload_buffer, ",");
        append_int(payload_buffer, sec); strcat(payload_buffer, "\n");
        
        strcat(payload_buffer, "S,"); strcat(payload_buffer, schedAlgo); strcat(payload_buffer, "\n");
        
        for (int j = 0; j < numThreads; j++) {
            strcat(payload_buffer, "T,"); append_int(payload_buffer, j + 1); strcat(payload_buffer, ",");
            strcat(payload_buffer, allThreads[j].name); strcat(payload_buffer, ",");
            strcat(payload_buffer, allThreads[j].current_state); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].priority); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].deadline); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].period); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].targetExecs); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].doneExecs); strcat(payload_buffer, ",");
            append_int(payload_buffer, allThreads[j].lastTAT); strcat(payload_buffer, "\n");
        }
        
        strcat(payload_buffer, "E,"); 
        append_int(payload_buffer, espInstance.reachable); strcat(payload_buffer, ",");
        append_int(payload_buffer, espInstance.op_mode); strcat(payload_buffer, ",");
        strcat(payload_buffer, espInstance.router_ssid[0] != '\0' ? espInstance.router_ssid : "N/A"); strcat(payload_buffer, ",");
        strcat(payload_buffer, espInstance.router_mac[0] != '\0' ? espInstance.router_mac : "N/A"); strcat(payload_buffer, ",");
        strcat(payload_buffer, espInstance.esp_ip[0] != '\0' ? espInstance.esp_ip : "N/A"); strcat(payload_buffer, ",");
        strcat(payload_buffer, espInstance.esp_mac[0] != '\0' ? espInstance.esp_mac : "N/A"); strcat(payload_buffer, "\n");

        esp_tcp_client_send(target_ip, target_port, payload_buffer);
    }
}

void getTasksInfo(threadStatParams allThreads[])
{
    for (int i = 0; i < numThreads; i++) {
        allThreads[i].name = threads[i].name;
        allThreads[i].current_state = get_thread_state_name(threads[i].currentState);
        allThreads[i].deadline = threads[i].deadlineOffset_ms;
        allThreads[i].doneExecs = threads[i].executionsDone;
        allThreads[i].targetExecs = threads[i].executionsTarget; 
        allThreads[i].priority = threads[i].priority;
        allThreads[i].period = threads[i].period_ms; 
        allThreads[i].lastTAT = threads[i].lastTurnaroundTime_ms;
    }
}

void getEspInfo(espStatParams* espInstance)
{
    while (esp_ring_buffer_pop() != '\0') {
        __asm__ volatile("nop");
    }

    send_to_esp("AT\r\n"); 
    if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 3))
    {
        espInstance->reachable = true;
    }
    
    send_to_esp("AT+CWMODE?\r\n"); 
    if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 3))
    {
        const char* mode_ptr = strstr(raw_esp_response_buffer, "+CWMODE:");
        if (mode_ptr)
        {
            mode_ptr += 8; 
            if (*mode_ptr >= '1' && *mode_ptr <= '3') {
                espInstance->op_mode = *mode_ptr - '0';
            }
        }
    }
    
    if (espInstance->op_mode == 1) 
    {
        send_to_esp("AT+CWJAP?\r\n"); 
        if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 5))
        {
            const char* jap_ptr = strstr(raw_esp_response_buffer, "+CWJAP:\"");
            if (jap_ptr)
            {
                jap_ptr += 8; 
                int i = 0;
                while (*jap_ptr != '"' && *jap_ptr != '\0' && *jap_ptr != '\r' && *jap_ptr != '\n' && i < 31) {
                    espInstance->router_ssid[i++] = *jap_ptr++;
                }
                espInstance->router_ssid[i] = '\0';

                jap_ptr = strstr(jap_ptr, ",\"");
                if (jap_ptr) {
                    jap_ptr += 2; 
                    i = 0;
                    while (*jap_ptr != '"' && *jap_ptr != '\0' && *jap_ptr != '\r' && *jap_ptr != '\n' && i < 17) {
                        espInstance->router_mac[i++] = *jap_ptr++;
                    }
                    espInstance->router_mac[i] = '\0';
                }
            }   
        }
        
        send_to_esp("AT+CIFSR\r\n");
        if(get_raw_esp_response(raw_esp_response_buffer, sizeof(raw_esp_response_buffer), 5))
        {
            const char* ip_ptr = strstr(raw_esp_response_buffer, "+CIFSR:STAIP,\"");
            if (!ip_ptr)
            {
                ip_ptr = strstr(raw_esp_response_buffer, "+CIFSR:APIP,\"");
                if (ip_ptr) ip_ptr += 13; 
            }
            else
            {
                ip_ptr += 14; 
            }

            if (ip_ptr)
            {
                int i = 0;
                while (*ip_ptr != '"' && *ip_ptr != '\0' && *ip_ptr != '\r' && *ip_ptr != '\n' && i < 15) {
                    espInstance->esp_ip[i++] = *ip_ptr++;
                }
                espInstance->esp_ip[i] = '\0';
            }

            const char* mac_ptr = strstr(raw_esp_response_buffer, "+CIFSR:STAMAC,\"");
            if (!mac_ptr) {
                mac_ptr = strstr(raw_esp_response_buffer, "+CIFSR:APMAC,\"");
                if (mac_ptr) mac_ptr += 14; 
            } else {
                mac_ptr += 15; 
            }

            if (mac_ptr)
            {
                int i = 0;
                while (*mac_ptr != '"' && *mac_ptr != '\0' && *mac_ptr != '\r' && *mac_ptr != '\n' && i < 17) {
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

    if (max_len > 0) {
        for(int i=0; i<max_len; i++) {
            buffer[i]='\0';
        }
    }

    uint64_t targetClockTick = sysctrGetTicks() + timeout_sec * sysctrGetFreq(); 

    while (sysctrGetTicks() < targetClockTick) {
        
        c = esp_ring_buffer_pop();
        
        if (c != '\0') {
            if (idx < max_len - 1) {
                buffer[idx++] = c;
                buffer[idx] = '\0';
            }

            if (prev == 'O' && c == 'K') {
                uint64_t flush_target = sysctrGetTicks() + (sysctrGetFreq() / 100); 
                while (sysctrGetTicks() < flush_target) {
                    char flush_c = esp_ring_buffer_pop(); 
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
            
            if (prev == 'O' && c == 'R') {
                success = false;
                timed_out = false;
                break;
            }

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

/* Retained for backward compatibility */
void build_stats_string(char* buffer, threadStatParams allThreads[], espStatParams espInstance)
{
    strcat(buffer, "--- THREADS ---\n");
    for(int i=0; i<numThreads; i++)
    {
        strcat(buffer, "\nThread "); append_int(buffer, (i+1)); strcat(buffer, "\n");
        
        strcat(buffer, "Name: "); strcat(buffer, allThreads[i].name); strcat(buffer, "\n");
        strcat(buffer, "State: "); strcat(buffer, allThreads[i].current_state); strcat(buffer, "\n");
        
        strcat(buffer, "Priority: "); append_int(buffer, allThreads[i].priority); strcat(buffer, "\n");
        strcat(buffer, "Deadline: "); append_int(buffer, allThreads[i].deadline); strcat(buffer, "\n");
        strcat(buffer, "Period: "); append_int(buffer, allThreads[i].period); strcat(buffer, "\n");
        
        strcat(buffer, "Target Execs: "); append_int(buffer, allThreads[i].targetExecs); strcat(buffer, "\n");
        strcat(buffer, "Done Execs: "); append_int(buffer, allThreads[i].doneExecs); strcat(buffer, "\n");
        strcat(buffer, "Last TAT: "); append_int(buffer, allThreads[i].lastTAT); strcat(buffer, " ms\n");
    }

    strcat(buffer, "\n--- ESP STATUS ---\n");
    if(espInstance.reachable == true) strcat(buffer, "Reachable: true\n");
    else strcat(buffer, "Reachable: false\n");
    
    strcat(buffer, "Op Mode: "); append_int(buffer, espInstance.op_mode); strcat(buffer, "\n");
    
    strcat(buffer, "Router SSID: "); 
    if(espInstance.router_ssid[0] != '\0') strcat(buffer, espInstance.router_ssid);
    else strcat(buffer, "N/A");
    strcat(buffer, "\n");

    strcat(buffer, "Router MAC: "); 
    if(espInstance.router_mac[0] != '\0') strcat(buffer, espInstance.router_mac);
    else strcat(buffer, "N/A");
    strcat(buffer, "\n");

    strcat(buffer, "ESP IP: "); 
    if(espInstance.esp_ip[0] !='\0') strcat(buffer, espInstance.esp_ip);
    else strcat(buffer, "N/A");
    strcat(buffer, "\n");

    strcat(buffer, "ESP MAC: "); 
    if(espInstance.esp_mac[0] !='\0') strcat(buffer, espInstance.esp_mac);
    else strcat(buffer, "N/A");
    strcat(buffer, "\n");
}
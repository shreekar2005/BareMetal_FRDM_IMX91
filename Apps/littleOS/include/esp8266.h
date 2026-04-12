#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>
#include "LPUART.h"

/**
 * @brief Initializes the ESP8266 Wi-Fi module's hardware interrupts.
 * This function registers the LPUART4 RX ISR with the OS, enables the interrupt in the GIC, and configures the LPUART4 hardware to generate interrupts when data is received from the ESP8266. Must be called once during system initialization.
 */
void esp_init(void);

/**
 * @brief Queries the ESP8266 for its current Wi-Fi mode, IP address, and TCP connections.
 */
void print_esp_status(void);

/**
 * @brief Configures the ESP8266 to act as a TCP Server.
 * @param port The network port to listen on (e.g., 8080).
 */
void start_esp_tcp_server(int port);

/**
 * @brief Configures the ESP8266 as an Access Point (AP).
 * You can connect your phone/laptop to this network.
 * * @param ssid The name of the Wi-Fi network to broadcast.
 * @param password The password for the network (must be >= 8 characters).
 */
void init_esp_as_access_point(const char* ssid, const char* password);

/**
 * @brief Configures the ESP8266 as a Station (STA).
 * It will connect to an existing Wi-Fi router.
 * * @param ssid The name of the router to connect to.
 * @param password The password of the router.
 */
void init_esp_as_station(const char* ssid, const char* password);

/**
 * @brief Acts as a TCP client to send a raw string to a specific IP and Port.
 * @param ip Destination IP address (e.g., "192.168.1.50")
 * @param port Destination Port (e.g., 5000)
 * @param payload The data to send (e.g., "GET_TIME")
 */
void esp_tcp_client_send(const char* ip, int port, const char* payload);

/* * Background RTOS Task to listen for RX from ESP8266, we can execute CLI commands from remote clients */
void espTCPServerListener_thread(void *arg);

#endif /* ESP8266_H */
#ifndef ESP8266_H
#define ESP8266_H

#include "LPUART.h"
#include "stdint.h"

/**
 * @brief Configures the ESP8266 to act as a TCP Server.
 * @param port The network port to listen on (e.g., 8080).
 */
void init_esp_tcp_server(int port);

/**
 * @brief Configures the ESP8266 as an Access Point (AP).
 * You can connect your phone/laptop to this network.
 * * @param ssid The name of the Wi-Fi network to broadcast.
 * @param password The password for the network (must be >= 8 characters).
 */
void init_esp_access_point(const char* ssid, const char* password);

/**
 * @brief Configures the ESP8266 as a Station (STA).
 * It will connect to an existing Wi-Fi router.
 * * @param ssid The name of the router to connect to.
 * @param password The password of the router.
 */
void init_esp_station(const char* ssid, const char* password);



#endif /* ESP8266_H */
#ifndef DATETIME_H
#define DATETIME_H

/**
 * @brief Background daemon thread that increments the system time.
 * This function runs infinitely, sleeping for 1000ms between ticks,
 * and handles the carry-over logic for seconds, minutes, hours, days, 
 * months, and years.
 * @param arg Optional thread argument (currently unused, pass NULL).
 */
void datetime_ticker_thread(void* arg);

/**
 * @brief Parses the raw command string and executes the requested RTC command.
 * Acts as the entry point for the CLI to interface with the datetime subsystem.
 * @param cmd The raw command string passed from the CLI.
 */
void datetime_handlecmd(const char* cmd);

/**
 * @brief Prints the current system date and time to the debug console.
 * Formats the terminal output strictly as "hh:mm:ss  dd/mm/yyyy".
 */
void datetime_show(void);

/**
 * @brief Manually overrides and sets the system clock.
 * @param arg1 The time string in strict "hh:mm:ss" format.
 * @param arg2 The date string in strict "dd:mm:yyyy" format.
 */
void datetime_set(const char* arg1, const char* arg2);

/**
 * @brief Synchronizes the system time with a remote TCP server.
 * Pauses the RTOS scheduler, connects to the specified IP and Port, 
 * and requests the current time using the "GET_TIME" payload.
 * @param arg1 Target server IP address (e.g., "192.168.21.103").
 * @param arg2 Target server TCP port (e.g., "5555").
 */
void datetime_sync(const char* arg1, const char* arg2);

#endif // DATETIME_H
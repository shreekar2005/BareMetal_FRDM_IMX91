#ifndef CLI_UTILITY_H
#define CLI_UTILITY_H

/**
 * @brief triggers hardware watchdog reset
 */
void system_reboot(void);

/**
 * @brief sends power down signal to pmic
 */
void system_poweroff(void);

/**
 * @brief clears the terminal screen using ANSI escape codes
 */
void clear_terminal(void);

/**
 * @brief prints the beautifully formatted rtos task manager table
 */
void print_stat(void);

#endif
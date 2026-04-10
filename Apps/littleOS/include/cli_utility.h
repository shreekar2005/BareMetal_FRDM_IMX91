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
void print_taskinfo(void);

/***
 * @brief prints the help menu with available commands and dynamically loaded tasks
 */
void print_help(void);

/**
 * @brief the main command handler that matches user input to registered commands and executes them
 * @param cmd the null-terminated user input string to parse and execute
 */
void handleCommand(const char* cmd);

#endif
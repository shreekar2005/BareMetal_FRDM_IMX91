#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

/**
 * @brief the main parser loop that handles user input from the lpuart terminal
 * @param arg thread arguments (unused)
 */
void input_thread(void* arg);

#endif
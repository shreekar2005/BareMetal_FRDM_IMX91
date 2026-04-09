#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

extern int cli_thread_id; /**< holds the thread id assigned to the command line interface */
extern int led_blink_thread_id; /**< holds the thread id assigned to the led blinking job */
extern int print100X_thread_id; /**< holds the thread id assigned to the print 50 X job */
extern int print100o_thread_id; /**< holds the thread id assigned to the print 50 o job */
extern int atomic_print100A_thread_id; /**< holds the thread id assigned to the atomic print job */
extern int echo_thread_id; /**< holds the thread id assigned to the echo job */

/**
 * @brief the main parser loop that handles user input from the lpuart terminal
 * @param arg thread arguments (unused)
 */
void input_thread(void* arg);

#endif
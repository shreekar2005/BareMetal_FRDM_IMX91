#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

extern volatile bool os_halt; /**< stops os loop if true */

extern volatile char print_buffer[128]; /**< buffer for worker thread to print */
extern volatile int print_count; /**< how many times left to print */

extern int cli_thread_id; /**< holds thread id for cli */
extern int led_blink_thread_id; /**< holds thread id for led */
extern int print_thread_id; /**< holds thread id for print worker */
extern int atomic_print_thread_id; /**< holds thread id for atomic print worker */

/**
 * @brief handles user input from terminal
 * @param arg thread argument
 */
void input_thread(void* arg);

#endif
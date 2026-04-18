// Task_Name : LED Control

#include <stddef.h>
#include "GPIO.h"
#include "include/multitasking.h"
#include "include/stdio.h"
#include "include/common_macros.h"

int ledblink_frequency_hz = 1; // default 1 Hz

void led_thread(void* arg) {
    print_dbg("\n");
    char* cmd_string = (char*)arg;
    if (cmd_string == NULL) return;

    char cmd[16] = {0}; // on, off, blink
    char arg1[32] = {0}; // color
    char arg2[32] = {0}; // frequency (if blink mode)
    int ptr = 0;
    int i = 0;

    // main command (on, off, blink)
    while (cmd_string[ptr] == ' ') ptr++;
    while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 15) {
        cmd[i++] = cmd_string[ptr++];
    }
    cmd[i] = '\0';

    // arg1 (color)
    i = 0;
    while (cmd_string[ptr] == ' ') ptr++;
    while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 31) {
        arg1[i++] = cmd_string[ptr++];
    }
    arg1[i] = '\0';

    // arg2 (frequency if blink mode)
    i = 0;
    while (cmd_string[ptr] == ' ') ptr++;
    while (cmd_string[ptr] != ' ' && cmd_string[ptr] != '\0' && i < 31) {
        arg2[i++] = cmd_string[ptr++];
    }
    arg2[i] = '\0';

    if (strcmp(cmd, "on") == 0) {
        if (strcmp(arg1, "red") == 0) {
            gpioWrite(GPIO2, BUILTIN_RED_LED, HIGH);
        } else if (strcmp(arg1, "green") == 0) {
            gpioWrite(GPIO2, BUILTIN_GREEN_LED, HIGH);
        } else if (strcmp(arg1, "blue") == 0) {
            gpioWrite(GPIO2, BUILTIN_BLUE_LED, HIGH);
        } else {
            print_dbg("[LED-Thread] Unknown color: %s\n", arg1);
        }


    } else if (strcmp(cmd, "off") == 0) {
        if (strcmp(arg1, "red") == 0) {
            gpioWrite(GPIO2, BUILTIN_RED_LED, LOW);
        } else if (strcmp(arg1, "green") == 0) {
            gpioWrite(GPIO2, BUILTIN_GREEN_LED, LOW);
        } else if (strcmp(arg1, "blue") == 0) {
            gpioWrite(GPIO2, BUILTIN_BLUE_LED, LOW);
        } else {
            print_dbg("[LED-Thread] Unknown color: %s\n", arg1);
        }


    } else if (strcmp(cmd, "blink") == 0) {
        if(atoi(arg2) > 0) ledblink_frequency_hz = atoi(arg2);
        if (ledblink_frequency_hz <= 0) ledblink_frequency_hz = 1; // default to 1 Hz
        uint32_t delay_ms = 500 / ledblink_frequency_hz; // on for half the period
        if (strcmp(arg1, "red") == 0) {
            while (1) {
                delay_ms = 500 / ledblink_frequency_hz; // recalculate in case frequency was changed externally
                gpioWrite(GPIO2, BUILTIN_RED_LED, HIGH);
                thread_sleep(delay_ms);
                gpioWrite(GPIO2, BUILTIN_RED_LED, LOW);
                thread_sleep(delay_ms);
            }
        } else if (strcmp(arg1, "green") == 0) {
            while (1) {
                delay_ms = 500 / ledblink_frequency_hz; // recalculate in case frequency was changed externally
                gpioWrite(GPIO2, BUILTIN_GREEN_LED, HIGH);
                thread_sleep(delay_ms);
                gpioWrite(GPIO2, BUILTIN_GREEN_LED, LOW);
                thread_sleep(delay_ms);
            }
        } else if (strcmp(arg1, "blue") == 0) {
            while (1) {
                delay_ms = 500 / ledblink_frequency_hz; // recalculate in case frequency was changed externally
                gpioWrite(GPIO2, BUILTIN_BLUE_LED, HIGH);
                thread_sleep(delay_ms);
                gpioWrite(GPIO2, BUILTIN_BLUE_LED, LOW);
                thread_sleep(delay_ms);
            }
        } else {
            print_dbg("[LED-Thread] Unknown color: %s\n", arg1);
        }
        
    } else {
        print_dbg("[LED-Thread] Invalid argument.\n");
        print_dbg("[LED-Thread] Usage:\n");
        print_dbg("[LED-Thread]   led on <color>\n");
        print_dbg("[LED-Thread]   led off <color>\n");
        print_dbg("[LED-Thread]   led blink <color> [frequency]\n");
        print_dbg("[LED-Thread] Colors: red, green, blue\n");
        print_dbg("[LED-Thread] Frequency is in Hz (times per second)\n");
    }
}
#include <stdint.h>
#include "GPIO.h"
#include "LPUART.h"

/**
 * @brief Main entry point for sonar_proximity
 */
int main() {
    /* Initialize your hardware here */
    uart_print_string(LPUART1, "Hello from sonar_proximity bare-metal!\n");
    return 0;
}
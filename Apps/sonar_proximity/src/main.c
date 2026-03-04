// main.c
#include <stdint.h>
#include "nxp_frdm_imx91.h"
#include "uart.h"

/**
 * @brief Main entry point for sonar_proximity
 */
int main() {
    /* Initialize your hardware here */
    uart_print_string(LPUART1, "Hello from sonar_proximity bare-metal!\n");
    return 0;
}
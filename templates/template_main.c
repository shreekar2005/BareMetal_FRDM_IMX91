// main.c
#include <stdint.h>
#include "nxp_frdm_imx91.h"
#include "uart.h"

/**
 * @brief Main entry point for __APP_NAME__
 */
int main() {
    /* Initialize your hardware here */
    uart_print_string(LPUART1, "Hello from __APP_NAME__ bare-metal!\n");
    return 0;
}
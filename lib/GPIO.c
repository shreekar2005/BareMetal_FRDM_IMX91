#include "GPIO.h"

/**
 * @brief Enables the clock for the GPIO block. (NOT FULLY IMPLEMENTED)
 * @param gpio Pointer to the GPIO instance (e.g. GPIO1).
 */
static void gpioEnableClock(GPIO_TypeDef *gpio) {
    /** i.MX91 Specific Note: 
     * Actual clock enablement on the i.MX9x series usually requires interacting 
     * with the CCM (Clock Control Module) LPCG registers. 
     * E.g., CCM->LPCG[GPIOx_INDEX] = 1;
     * Placeholder below for standard clock enablement.
     */
    if (gpio == GPIO1) {
        // Enable GPIO1 clock in CCM
    } else if (gpio == GPIO2) {
        // Enable GPIO2 clock in CCM
    } else if (gpio == GPIO3) {
        // Enable GPIO3 clock in CCM
    } else if (gpio == GPIO4) {
        // Enable GPIO4 clock in CCM
    }
    
}

void gpioPinINIT(GPIO_TypeDef *gpio, uint8_t pin, GPIO_PinMode mode) {
    /* Note: ANALOG and ALTERNATE_FUNCTION modes are intentionally ignored here.
     * They must be configured using the IOMUXC via iomuxSetPadAltMode().
     */
    if (pin > 31) return;
    gpioEnableClock(gpio);
    if (mode == INPUT_MODE)
        gpio->PDDR &= ~(1 << pin); // Clear bit in Data Direction Register to set as input
    else if (mode == OUTPUT_MODE)
        gpio->PDDR |= (1 << pin); // Set bit in Data Direction Register to set as output
}

void gpioWrite(GPIO_TypeDef *gpio, uint8_t pin, uint8_t value) {
    if (pin > 31) return;

    if (value == HIGH) {
        /* Write 1 to Port Set Output Register (PSOR) to drive pin high.
         * Writing 0 to other bits in PSOR has no effect. */
        gpio->PSOR = (1 << pin);
    } else {
        /* Write 1 to Port Clear Output Register (PCOR) to drive pin low.
         * Writing 0 to other bits in PCOR has no effect. */
        gpio->PCOR = (1 << pin);
    }
}

uint8_t gpioRead(GPIO_TypeDef *gpio, uint8_t pin) {
    if (pin > 31) return LOW; // Bounds check

    if (gpio->PDIR & (1 << pin)) {
        return HIGH;
    } else {
        return LOW;
    }
}
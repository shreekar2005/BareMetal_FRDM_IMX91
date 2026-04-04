#include "GPIO.h"

void initGPIO(GPIO_TypeDef *gpio) {
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
    
    gpio->PDOR = 0x00000000; // Reset default outputs to 0
}

void setPinMode(GPIO_TypeDef *gpio, uint8_t pin, GPIO_PinMode mode) {
    if (pin > 31) return;

    switch (mode) {
        case INPUT_MODE:
            gpio->PDDR &= ~(1 << pin); // Clear bit in Data Direction Register to set as input
            break;
            
        case OUTPUT_MODE:
            gpio->PDDR |= (1 << pin); // Set bit in Data Direction Register to set as output
            break;
            
        case ANALOG_MODE:
        case ALTERNATE_FUNCTION_MODE:
            /** i.MX91 Specific Note:
             * Setting a pin to Analog or an Alternate Function (AF) is handled 
             * by the IOMUXC (I/O Multiplexer Controller), not the GPIO block itself.
             * Example:
             * IOMUXC->SW_MUX_CTL_PAD[PIN_INDEX] = AF_MUX_MODE;
             */
            break;
    }
}
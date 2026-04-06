#ifndef IOMUX_H
#define IOMUX_H

#include <stdint.h>

#define IOMUXC_BASE 0x443C0000

/* Pad Mux Registers for P11 Expansion Header Pins */
#define MUX_REG_GPIO_IO14    (IOMUXC_BASE + 0x0048) 
#define MUX_REG_GPIO_IO15    (IOMUXC_BASE + 0x004C)

/* DAISY Registers */
#define DAISY_REG_LPUART3_RX (IOMUXC_BASE + 0x0470)
#define DAISY_REG_LPUART4_RX (IOMUXC_BASE + 0x047C)

#define AF_MODE_LPUART3  1 
#define AF_MODE_LPUART4  6

#define DAISY_VALUE_IO15_LPUART3 0
#define DAISY_VALUE_IO15_LPUART4 0


/**
 * @brief Routes the physical pad to a specific peripheral via IOMUXC.
 * @param mux_reg_address The physical memory address of the IOMUXC SW_MUX_CTL_PAD register.
 * @param af_mode The Alternate Function number (e.g., 1 for ALT1).
 * @param sion Software Input On (0 to disable, 1 to force input path).
 */
void setPinMux(uintptr_t mux_reg_address, uint8_t af_mode, uint8_t sion);

#endif /* IOMUX_H */
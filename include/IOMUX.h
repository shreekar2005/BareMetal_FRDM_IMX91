/**
 * Here we only implemented IOMUX which we require in littleOS. We can expand this in the future as needed. The main purpose of this file is to provide a clean and simple interface for configuring the IOMUXC registers, abstracting away the low-level details of bit manipulation and register access. This allows us to easily set up pin multiplexing for peripherals like UART without having to worry about the specific bits in the IOMUXC registers each time.
 */

#ifndef IOMUX_H
#define IOMUX_H

#include <stdint.h>

#define IOMUXC_BASE 0x443C0000

/* Pad Mux Registers for P11 Expansion Header Pins */
#define MUX_REG_GPIO_IO14    (IOMUXC_BASE + 0x0048) 
#define MUX_REG_GPIO_IO15    (IOMUXC_BASE + 0x004C)

#define ALT_MODE_LPUART4  6

/** We require daisy registers to listen to perticular pad if multiple pads are assigned to single pin of periferal.
 * Listening to multiple pads can corrupt input data on periferal pin.
 */

/* DAISY Registers */
#define DAISY_REG_LPUART4_RX (IOMUXC_BASE + 0x047C)

/* DAISY Registers values*/
#define DAISY_VALUE_GPIO_IO15_LPUART4 0


/**
 * @brief Routes the physical pad to a specific peripheral via IOMUXC.
 * @param mux_reg_address The physical memory address of the IOMUXC SW_MUX_CTL_PAD register.
 * @param alt_mode The Alternate Function number (e.g., 1 for ALT1).
 * @param sion Software Input On (0 to disable, 1 to force input path).
 */
void iomuxSetPadAltMode(uintptr_t mux_reg_address, uint8_t alt_mode, uint8_t sion);

#endif /* IOMUX_H */
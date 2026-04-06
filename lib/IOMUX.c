#include "IOMUX.h"

#include "IOMUX.h"

void setPinMux(uintptr_t mux_reg_address, uint8_t af_mode, uint8_t sion) {
    /* Cast the physical address to a volatile pointer */
    volatile uint32_t *mux_reg = (volatile uint32_t *)mux_reg_address;
    
    /* Read the current register value */
    uint32_t reg_value = *mux_reg;
    
    /* Clear ONLY the MUX_MODE (bits 2-0) and SION (bit 4) 
     * clears bits 0,1,2,4 */
    reg_value &= ~0x17;
    
    /* Insert the new MUX_MODE and SION values */
    reg_value |= (af_mode & 0x07);       // Insert 3-bit MUX_MODE at bit 0
    reg_value |= ((sion & 0x01) << 4);   // Insert 1-bit SION at bit 4
    
    /* Write the safely modified value back to the Pad Mux Register */
    *mux_reg = reg_value; 
}
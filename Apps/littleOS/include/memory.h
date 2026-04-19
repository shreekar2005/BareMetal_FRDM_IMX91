#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Linker symbols defined in the linker script.

extern uint8_t __text_start[];
extern uint8_t __text_end[];

extern uint8_t __rodata_start[];
extern uint8_t __rodata_end[];

extern uint8_t __data_start[];
extern uint8_t __data_end[];

extern uint8_t __bss_start[];
extern uint8_t __bss_end[];

/**
 * @brief priting all memory sections size used by kernel binary
 */
void memory_print_footprint(void);

#endif /* MEMORY_H */
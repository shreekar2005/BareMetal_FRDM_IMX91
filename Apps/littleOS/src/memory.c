#include "include/memory.h"
#include "include/stdio.h"

void memory_print_footprint(void) {
    uintptr_t text_size   = (uintptr_t)__text_end - (uintptr_t)__text_start;
    uintptr_t rodata_size = (uintptr_t)__rodata_end - (uintptr_t)__rodata_start;
    uintptr_t data_size   = (uintptr_t)__data_end - (uintptr_t)__data_start;
    uintptr_t bss_size    = (uintptr_t)__bss_end - (uintptr_t)__bss_start;
    
    uintptr_t total_size  = text_size + rodata_size + data_size + bss_size;

    print_dbg("[MEM-Driver] littleOS Memory Footprint:\n");
    print_dbg("[MEM-Driver] Section   | Start Address | End Address   | Size\n");
    print_dbg("[MEM-Driver] --------------------------------------------------------\n");
    
    print_dbg("[MEM-Driver] .bss      | 0x%08X    | 0x%08X    | %.2f KB\n", 
              (uint32_t)(uintptr_t)__bss_start, (uint32_t)(uintptr_t)__bss_end, (float)bss_size / 1024.0f);
                  
    print_dbg("[MEM-Driver] .data     | 0x%08X    | 0x%08X    | %.2f KB\n", 
              (uint32_t)(uintptr_t)__data_start, (uint32_t)(uintptr_t)__data_end, (float)data_size / 1024.0f); 
    
    print_dbg("[MEM-Driver] .rodata   | 0x%08X    | 0x%08X    | %.2f KB\n", 
              (uint32_t)(uintptr_t)__rodata_start, (uint32_t)(uintptr_t)__rodata_end, (float)rodata_size / 1024.0f);

    print_dbg("[MEM-Driver] .text     | 0x%08X    | 0x%08X    | %.2f KB\n", 
              (uint32_t)(uintptr_t)__text_start, (uint32_t)(uintptr_t)__text_end, (float)text_size / 1024.0f);
       
    print_dbg("[MEM-Driver] --------------------------------------------------------\n");
    print_dbg("[MEM-Driver] Total littleOS Binary Size in RAM: %.2f KB\n", (float)total_size / 1024.0f);
}
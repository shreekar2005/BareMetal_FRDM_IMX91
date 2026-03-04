/* linker.ld */
ENTRY(_start)
SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.start)
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}
ENTRY(_start)
SECTIONS
{
    . = 0x80000000;
    
    .text : {
        __text_start = .;
        *(.text.start)
        *(.text*)
        __text_end = .;
    }

    . = ALIGN(16);
    .rodata : { 
        __rodata_start = .;
        *(.rodata*) 
        __rodata_end = .;
    }

    . = ALIGN(16);
    .data : { 
        __data_start = .;
        *(.data*) 
        __data_end = .;
    }

    . = ALIGN(16);
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16); /* Keeps the end of the section aligned for the assembly loop */
        __bss_end = .;
    }
}
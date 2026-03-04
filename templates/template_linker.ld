ENTRY(_start)
SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.start)
        *(.text*)
    }

    . = ALIGN(16);
    .rodata : { 
        *(.rodata*) 
    }

    . = ALIGN(16);
    .data : { 
        *(.data*) 
    }
    
    . = ALIGN(16);
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16); 
        __bss_end = .;
    }
}
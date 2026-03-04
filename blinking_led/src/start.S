// start.S
.global _start

_start:
    /* Set up the stack pointer in DDR RAM */
    ldr x0, =0x81000000 
    mov sp, x0

    /* Jump to C main */
    bl main

hang:
    wfe
    b hang
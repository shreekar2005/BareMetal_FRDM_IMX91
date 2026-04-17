.extern main
.extern __bss_start
.extern __bss_end

.global _start

.section .text.start

_start:
    ldr x0, =__bss_start
    ldr x1, =__bss_end

bss_clear_loop:
    cmp x0, x1
    b.eq bss_clear_done
    str xzr, [x0], #8 
    b bss_clear_loop

bss_clear_done:
    bl main
    b .
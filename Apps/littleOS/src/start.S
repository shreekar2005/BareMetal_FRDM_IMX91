.global _start
.extern main
.extern __bss_start
.extern __bss_end

.section .text.start

_start:
    // 1. U-Boot already set up a safe stack pointer for us.
    // We just need to zero out the .bss section so global variables 
    // like 'num_threads' and 'os_halt' initialize cleanly to 0.
    ldr x0, =__bss_start
    ldr x1, =__bss_end

bss_clear_loop:
    cmp x0, x1
    b.eq bss_clear_done
    str xzr, [x0], #8  // Store zero and advance pointer by 8 bytes
    b bss_clear_loop

bss_clear_done:
    // 2. Jump to our C code!
    bl main

    // 3. Fallback trap if main ever returns
    b .
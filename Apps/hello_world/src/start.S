// start.S
.global _start
.extern __bss_start
.extern __bss_end

_start:
   stp x29, x30, [sp, #-16]!  /* Save U-Boot's FP and LR */
   stp x19, x20, [sp, #-16]!  /* Back up U-Boot's x19 and x20 */
   mov x19, sp                /* Save U-Boot's Stack Pointer into x19 */

   /* Set up our own stack pointer at a safe DDR address */
   ldr x0, =0x81000000 
   mov sp, x0

   /* Clear the .bss section (Zero-initialize global variables) */
   ldr x1, =__bss_start
   ldr x2, =__bss_end
    
.bss_clear_loop:
   cmp x1, x2                 /* Have we reached the end of BSS? */
   b.ge .bss_clear_done       /* If yes, exit loop */
   str xzr, [x1], #8          /* Store 0 (xzr), then increment address by 8 */
   b .bss_clear_loop

.bss_clear_done:
   bl main                    /* Jump to C. Return value will be in x0 */

   mov sp, x19                /* Give the stack pointer back to U-Boot */
   ldp x19, x20, [sp], #16    /* Restore U-Boot's x19 and x20 */
   ldp x29, x30, [sp], #16    /* Restore U-Boot's FP and LR */
   
   ret /* Return to U-Boot. x0 holds main's return code. */
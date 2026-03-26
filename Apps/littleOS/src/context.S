.global os_yield
.extern schedule_tick

.section .text
.align 2

os_yield:
    // 1. Make room on the stack (272 bytes)
    sub sp, sp, #272

    // 2. Save X0-X29
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]

    // 3. Save LR (X30)
    str x30, [sp, #240]
    
    // 4. In a cooperative yield, we want to resume exactly where we left off (LR)
    str x30, [sp, #248] // Save LR into the 'PC' slot
    
    // 5. Save fake CPSR and original SP
    str xzr, [sp, #256] 
    add x0, sp, #272
    str x0, [sp, #264]

    // 6. Call the C++ Scheduler!
    mov x0, sp 
    bl schedule_tick
    mov sp, x0 // Replace SP with the returned thread's SP

    // 7. Restore Registers (Leaving x9 free for our PC branch)
    ldp x28, x29, [sp, #224]
    ldp x26, x27, [sp, #208]
    ldp x24, x25, [sp, #192]
    ldp x22, x23, [sp, #176]
    ldp x20, x21, [sp, #160]
    ldp x18, x19, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9,   [sp, #64]
    ldp x6, x7,   [sp, #48]
    ldp x4, x5,   [sp, #32]
    ldp x2, x3,   [sp, #16]

    // 8. Grab the Target PC and LR
    ldr x30, [sp, #240] // Restore LR
    ldr x9, [sp, #248]  // Load PC into x9 (volatile scratch register)

    // 9. Restore final registers and revert stack
    ldp x0, x1, [sp, #0]
    add sp, sp, #272

    // 10. Branch to the Thread's instruction!
    br x9

.global os_start
os_start:
    b os_yield
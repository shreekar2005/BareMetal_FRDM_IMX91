// These symbols are defined in other files
.extern irq_dispatcher
.extern os_fatal_error
.extern schedule

// These functions are meant to used in other files
.global vector_table
.global __os_start_asm

// CONTEXT SWITCH MACROS

.macro STORE_CONTEXT
    // 272 (GPRs + SysRegs) + 528 (FPU/NEON) = 800 bytes (around 1KB)
    sub sp, sp, #800

    // Save General Purpose Registers (Offsets 0 to 239)
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

    // Save Link Register (Offset 240)
    str x30, [sp, #240]

    // Save Exception/System Registers (Offsets 248, 256)
    mrs x0, elr_el2
    str x0, [sp, #248]

    mrs x1, spsr_el2
    str x1, [sp, #256]

    // Save original Stack Pointer (Offset 264)
    add x2, sp, #800 
    str x2, [sp, #264]

    // Save FPU Status and Control Registers (Offsets 272, 280)
    mrs x0, fpsr
    mrs x1, fpcr
    stp x0, x1, [sp, #272]

    // Save 128-bit NEON/FPU 'q' registers (Offsets 288 to 799)
    // stp with 'q' registers stores 32 bytes per instruction
    stp q0, q1, [sp, #288]
    stp q2, q3, [sp, #320]
    stp q4, q5, [sp, #352]
    stp q6, q7, [sp, #384]
    stp q8, q9, [sp, #416]
    stp q10, q11, [sp, #448]
    stp q12, q13, [sp, #480]
    stp q14, q15, [sp, #512]
    stp q16, q17, [sp, #544]
    stp q18, q19, [sp, #576]
    stp q20, q21, [sp, #608]
    stp q22, q23, [sp, #640]
    stp q24, q25, [sp, #672]
    stp q26, q27, [sp, #704]
    stp q28, q29, [sp, #736]
    stp q30, q31, [sp, #768]
.endm

.macro RESTORE_CONTEXT
    // Restore FPU 'q' registers
    ldp q30, q31, [sp, #768]
    ldp q28, q29, [sp, #736]
    ldp q26, q27, [sp, #704]
    ldp q24, q25, [sp, #672]
    ldp q22, q23, [sp, #640]
    ldp q20, q21, [sp, #608]
    ldp q18, q19, [sp, #576]
    ldp q16, q17, [sp, #544]
    ldp q14, q15, [sp, #512]
    ldp q12, q13, [sp, #480]
    ldp q10, q11, [sp, #448]
    ldp q8, q9,   [sp, #416]
    ldp q6, q7,   [sp, #384]
    ldp q4, q5,   [sp, #352]
    ldp q2, q3,   [sp, #320]
    ldp q0, q1,   [sp, #288]

    // Restore FPU Control & System Registers
    ldp x0, x1, [sp, #272]
    msr fpsr, x0
    msr fpcr, x1

    ldr x0, [sp, #248]
    ldr x1, [sp, #256]
    msr elr_el2, x0
    msr spsr_el2, x1

    // Restore Link Register
    ldr x30, [sp, #240]

    // Restore General Purpose Registers
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
    ldp x0, x1,   [sp, #0]

    // Shrink stack and return
    add sp, sp, #800
    eret
.endm

// Start of actual code
.section .text

.align 11 // Forces the table to a 2^11 (2048) byte boundary. VBAR_EL2 ignores the bottom 11 bits.
vector_table:

    // BLOCK 1: Exceptions from CURRENT privilege level, using the SP0 stack.
    // These should theoretically never fire unless gic misconfigured.
    
    .align 7 // Offset +0x000: Synchronous Exception (Data Abort, Undefined Instruction, ...)
    b trap_sync 

    .align 7 // Offset +0x080: Hardware IRQ (UART, Timer, ...) 
    b trap_irq 
    
    .align 7 // Offset +0x100: FIQ (Fast Interrupt)
    b trap_fiq 
    
    .align 7 // Offset +0x180: SError (System Bus Error, asynchronous memory faults)
    b trap_serror 

    // BLOCK 2: Exceptions from CURRENT privilege level, using the SPx stack. (U-BOOT sets this thing)
    // CPU jumps here when executing kernel thread.
    
    .align 7 // Offset +0x200: Synchronous Exception (Data Abort, Undefined Instruction, ...)
    b trap_sync 
    
    .align 7 // Offset +0x280: Hardware IRQ (UART, Timer, ...)
    b irq_handler 
    
    .align 7 // Offset +0x300: FIQ (Fast Interrupt)
    b trap_fiq 
    
    .align 7 // Offset +0x380: SError (System Bus Error, asynchronous memory faults, ...)
    b trap_serror 

    // BLOCK 3: Exceptions from LOWER privilege level running AArch64.
    // CPU jumps here if an EL0 user-space app triggers an exception/syscall.
    
    .align 7 // Offset +0x400: Synchronous
    b trap_sync 
    
    .align 7 // Offset +0x480: Hardware IRQ
    b irq_handler 
    
    .align 7 // Offset +0x500: FIQ
    b trap_fiq 
    
    .align 7 // Offset +0x580: SError
    b trap_serror 

    // BLOCK 4: Exceptions from LOWER privilege level running AArch32.
    // (Required to complete the 2048-byte table size)
    .align 7 
    b trap_sync 
    
    .align 7 
    b irq_handler 
    
    .align 7 
    b trap_fiq 
    
    .align 7 
    b trap_serror


// Exception Handlers

trap_sync:
    mov x3, #0
    b common_trap
trap_irq:
    mov x3, #1
    b common_trap
trap_fiq:
    mov x3, #2
    b common_trap
trap_serror:
    mov x3, #3
    b common_trap

irq_handler:
    STORE_CONTEXT
    mov x0, sp 
    bl irq_dispatcher 
    mov sp, x0
    RESTORE_CONTEXT


// Helpers for Exception Handlers

common_trap:
    msr daifset, #2      
    ldr x4, =0x80200000  
    mov sp, x4
    mrs x0, esr_el2
    mrs x1, elr_el2
    mrs x2, far_el2
    b os_fatal_error

__os_start_asm:
    mov x0, #0
    bl schedule
    mov sp, x0 
    RESTORE_CONTEXT
// These functions are meant to used in other files
.global vector_table

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
    
    .align 7 // Offset +0x280: Hardware IRQ (UART, Timer, ...) // YOU WILL NEED TO USE THIS TO USE INTERRUPTS
    b trap_irq 
    
    .align 7 // Offset +0x300: FIQ (Fast Interrupt)
    b trap_fiq 
    
    .align 7 // Offset +0x380: SError (System Bus Error, asynchronous memory faults, ...)
    b trap_serror 

    // BLOCK 3: Exceptions from LOWER privilege level running AArch64.
    // CPU jumps here if an EL0 user-space app triggers an exception/syscall.
    
    .align 7 // Offset +0x400: Synchronous
    b trap_sync 
    
    .align 7 // Offset +0x480: Hardware IRQ
    b trap_irq 
    
    .align 7 // Offset +0x500: FIQ
    b trap_fiq 
    
    .align 7 // Offset +0x580: SError
    b trap_serror 

    // BLOCK 4: Exceptions from LOWER privilege level running AArch32.
    // (Required to complete the 2048-byte table size)
    .align 7 
    b trap_sync 
    
    .align 7 
    b trap_irq 
    
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

common_trap:
    msr daifset, #2     // Disable further IRQs to avoid nested exceptions
    ldr x4, =0x80200000 // Load the address of emergency stack (THIS MAY VERY APP TO APP)
    mov sp, x4          // Switch to emergency stack to handle the exception safely
    mrs x0, esr_el2     // Exception Syndrome Register gives more info about the exception cause
    mrs x1, elr_el2     // Exception Link Register gives the address of the instruction that caused the exception
    mrs x2, far_el2     // For data aborts, gives the faulting virtual address. For other exceptions, value is undefined.
halt_cpu:
    wfi                 // Put CPU to sleep to save power
    b halt_cpu          // If it somehow wakes up, loop back and sleep again

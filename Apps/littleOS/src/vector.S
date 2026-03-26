.global vector_table
.global os_start
.extern irq_dispatcher
.extern os_fatal_error
.extern schedule_tick

.section .text
.align 11 

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
    msr daifset, #2      
    
    ldr x4, =0x80200000  
    mov sp, x4
    
    mrs x0, esr_el2
    mrs x1, elr_el2
    mrs x2, far_el2
    b os_fatal_error

.align 11
vector_table:
    b trap_sync 
    .align 7
    b trap_irq 
    .align 7
    b trap_fiq 
    .align 7
    b trap_serror 
    .align 7
    
    b trap_sync 
    .align 7
    
    b irq_handler 
    
    .align 7
    b trap_fiq 
    .align 7
    b trap_serror 

irq_handler:
    sub sp, sp, #272

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

    str x30, [sp, #240]

    mrs x0, elr_el2
    mrs x1, spsr_el2
    add x2, sp, #272 
    
    str x0, [sp, #248]
    str x1, [sp, #256]
    str x2, [sp, #264]

    mov x0, sp 
    bl irq_dispatcher 
    mov sp, x0

    ldr x0, [sp, #248]
    ldr x1, [sp, #256]
    msr elr_el2, x0
    msr spsr_el2, x1

    ldr x30, [sp, #240]

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

    add sp, sp, #272
    eret 

restore_context:
    ldr x0, [sp, #248]
    ldr x1, [sp, #256]
    msr elr_el2, x0
    msr spsr_el2, x1

    ldr x30, [sp, #240]

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

    add sp, sp, #272
    eret

os_start:
    mov x0, #0
    bl schedule_tick
    mov sp, x0 
    b restore_context
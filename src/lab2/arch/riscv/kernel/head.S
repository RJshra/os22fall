.extern start_kernel
.extern _traps

    .section .text.init
    .globl _start
_start:
     # set stvec = _traps
    la sp, boot_stack_top
    la s0, _traps
    csrw stvec, s0

    # set sie[STIE] = 1
    csrr s0, sie
    #stie是第6位
    ori s0, s0,0x0000000000000020
    csrw sie, s0

    # set first time interrupt
    rdtime t0
    li t1,10000000
    add t0,t0,t1
    li a7,0
    li a6,0
    mv a0,t0
    li a1,0
    li a2,0
    li a3,0
    li a4,0
    li a5,0
    ecall
    # set sstatus[SIE] = 1
    csrr s0, sstatus
    #sie是第2位
    ori s0,s0, 0x0000000000000002
    csrw sstatus, s0

    j start_kernel
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 

    .globl boot_stack_top
boot_stack_top:
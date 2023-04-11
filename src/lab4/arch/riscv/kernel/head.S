.extern start_kernel
.extern _traps

    .section .text.init
    .globl _start
_start:
     # set stvec = _traps
    la sp, boot_stack_top

    call setup_vm
    call relocate 

    call mm_init
    call setup_vm_final
    
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
    csrr t0, sstatus
    #sie是第2位
    ori t0, t0, 0x0000000000000002
    csrw sstatus, t0

    
    call task_init

    j start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    
    ###################### 
    #   YOUR CODE HERE   #
    ######################
    li t3,0xFFFFFFDF80000000
    add ra, ra, t3 
    add sp, sp, t3 

    # set satp with early_pgtbl
    la t0, early_pgtbl
    addi t1, x0, 0x8
    slli t1,t1,60
    #pa>>12=ppn,这里pa是页表地址
    srli t0,t0,12
    add t1,t1,t0
    csrw satp,t1 
    ###################### 
    #   YOUR CODE HERE   #
    ######################

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 

    .globl boot_stack_top
boot_stack_top:
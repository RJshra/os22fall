    .section .text.entry
    .align 2
    .globl _traps 

    .globl __dummy
    .globl __switch_to
.extern dummy

__dummy:
    csrr t0, sscratch
    csrw sscratch, sp
    addi sp, t0, 0
    sret

__switch_to:
    #save the state to prev process
    #task_struct位于低地址，向上储存
    addi t0,a0,40
    sd ra,0(t0)
    sd sp,8(t0)
    sd s0,16(t0)
    sd s1,24(t0)
    sd s2,32(t0)
    sd s3,40(t0)
    sd s4,48(t0)
    sd s5,56(t0)
    sd s6,64(t0)
    sd s7,72(t0)
    sd s8,80(t0)
    sd s9,88(t0)
    sd s10,96(t0)
    sd s11,104(t0)
    #sepc
    csrr t1, sepc
    sd t1, 112(t0)
    #sstatus
    csrr t1, sstatus
    sd t1, 120(t0)
    #sscratch
    csrr t1, sscratch
    sd t1, 128(t0)

    # restore state from next process
    addi t0,a1,40
    ld ra,0(t0)
    ld sp,8(t0)
    ld s0,16(t0)
    ld s1,24(t0)
    ld s2,32(t0)
    ld s3,40(t0)
    ld s4,48(t0)
    ld s5,56(t0)
    ld s6,64(t0)
    ld s7,72(t0)
    ld s8,80(t0)
    ld s9,88(t0)
    ld s10,96(t0)
    ld s11,104(t0)
    #sepc
    ld t1, 112(t0)
    csrw sepc, t1
    #sstatus
    ld t1, 120(t0)
    csrw sstatus,t1
    #sscratch
    ld t1, 128(t0)
    csrw sscratch,t1

    ld t1, 136(t0)
    srl t1,t1,12
    li t2,0x8000000000000000
    add t1,t1,t2
    csrw satp,t1
    
    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i
    ret


_traps:
    csrr t0,sscratch
    // if sccrath==0, in S mode
    beqz t0,_old_traps
    //else U mode
    csrw sscratch, sp
    addi sp, t0, 0
    
_old_traps:
    # -----------
    addi sp,sp,-304
    sd x0, 0(sp) 
    sd x1, 8(sp)
    sd x2, 16(sp)
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9,  72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    sd sp, 256(sp)
    #csrrw s0, sscratch, x0
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, scause
    csrr t0, stval
    csrr t1, sscratch
    sd s1, 264(sp)
    sd s2, 272(sp)
    sd s3, 280(sp)
    sd t0, 288(sp)
    sd t1, 296(sp)
    # -----------

    # 2. call trap_handler
    csrr a0,scause
    csrr a1,sepc
    mv a2,sp
    jal x1, trap_handler
    .global __ret_from_fork
__ret_from_fork: 
    # -----------

    # 3. restore sepc and 32 registers (x2(a0) should be restore last) from stack
    ld s1, 264(sp)
    ld s2, 272(sp)
    ld s3, 280(sp)
    ld t0, 288(sp)
    ld t1, 296(sp)
    csrw sstatus, s1
    csrw sepc, s2
    csrw scause, s3
    ld x0, 0(sp) 
    ld x1, 8(sp)
    //ld x2, 16(sp)
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)

    ld sp, 256(sp)
    addi sp,sp,304

    /// if sccrath==0, in S mode
    csrr t0, sscratch
    beqz t0, _return
    csrw sscratch, sp
    addi sp, t0, 0
    # -----------
_return:
        # 4. return from trap
    sret
    # -----------

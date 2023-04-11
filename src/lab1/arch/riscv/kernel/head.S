.extern start_kernel

    .section .text.entry
    .globl _start
_start:

    la sp, boot_stack_top
    j start_kernel
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 

    .globl boot_stack_top
boot_stack_top:
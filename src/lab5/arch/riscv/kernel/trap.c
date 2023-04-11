#include "sbi.h"
#include "printk.h"
#include "proc.h"
#include"defs.h"
#include"syscall.h"
extern void clock_set_next_event();

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    //time interrupt
    if((scause&0x8000000000000000)!=0){
        if((scause&0x7fffffffffffffff)==5){
            //printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
            //printk("int\n");
        }
    }
    else{
        //environmental call from U mode
        if((scause&0x8)!=0){
            //64 号系统调用sys_write
            if(regs->a7==SYS_WRITE){
                regs->a0 = sys_write(regs->a0, (const char*)regs->a1, regs->a2);
                regs->sepc+=4;
            }
            //172 号系统调用 sys_getpid() 该调用从current中获取当前的pid放入a0中返回
            else if(regs->a7==SYS_GETPID){
                regs->a0=sys_getpid();
                regs->sepc+=4;
            }
        }
    }

}
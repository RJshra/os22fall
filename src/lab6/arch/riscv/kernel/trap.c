#include "sbi.h"
#include "printk.h"
#include "proc.h"
#include"defs.h"
#include"syscall.h"
extern void clock_set_next_event();
extern uint64_t load_program(struct task_struct* task);
extern char uapp_start[];
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern void __ret_from_fork();
void do_page_fault(struct pt_regs *regs) {
    /*
     1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
     2. 通过 find_vma() 查找 Bad Address 是否在某个 vma 中
     3. 分配一个页，将这个页映射到对应的用户地址空间
     4. 通过 (vma->vm_flags | VM_ANONYM) 获得当前的 VMA 是否是匿名空间
     5. 根据 VMA 匿名与否决定将新的页清零或是拷贝 uapp 中的内容
    */
    //如果是匿名区域，那么开辟一页内存，然后把这一页映射到导致异常产生 task 的页表中。
    //如果不是，那么首先将硬盘中的内容读入 buffer pool，将 buffer pool 中这段内存映射给 task。
    uint64 stval=regs->stval;
    
    struct vm_area_struct* vma = find_vma(current,stval);
    if(vma==NULL){
        //
    }
    else{
        uint64 new_page=(uint64)kalloc();
        create_mapping((uint64*)((uint64_t)current->pgd+PA2VA_OFFSET),stval>>12<<12,new_page-PA2VA_OFFSET,PGSIZE,(vma->vm_flags)|0x11);
        //非匿名空间
        if(!(vma->vm_flags&VM_ANONYM)){
            char* page=(char*)new_page;
            char* source=(char*)((vma->vm_content_offset_in_file+(uint64)uapp_start+stval-vma->vm_start)>>12<<12);
            for(int i=0;i<PGSIZE;i++){
                page[i]=source[i];
            }
        }
    }
}

uint64_t sys_clone(struct pt_regs *regs) {
    /*
     1. 参考 task_init 创建一个新的 task, 将的 parent task 的整个页复制到新创建的 
        task_struct 页上(这一步复制了哪些东西?）。将 thread.ra 设置为 
        __ret_from_fork, 并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)

     2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
        并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)

     3. 为 child task 申请 user stack, 并将 parent task 的 user stack 
        数据复制到其中。 

     4. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射

     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存

     6. 返回子 task 的 pid
    */
    for(int i=2;i<NR_TASKS;i++){
        if(task[i]==NULL){
            task[i]=(struct task_struct*)kalloc();
            char* dist=(char*)task[i];
            char* src=(char*)(current);
            for(int j=0;j<PGSIZE;j++){
                dist[j]=src[j];
            }
            task[i]->state=TASK_RUNNING;
            task[i]->counter=0;
            task[i]->priority=rand();
            task[i]->pid=i;
            task[i]->thread.ra=(uint64)(&__ret_from_fork);
            task[i]->thread.sp=(uint64)task[i]+(uint64)regs-(uint64)current;
            //pt_regs
            task[i]->thread.sepc=regs->sepc+4;
            task[i]->thread.sscratch=regs->sscratch;
            task[i]->thread.sstatus=regs->sstatus;
            struct pt_regs *child_regs=(struct pt_regs*)(task[i]->thread.sp);
            child_regs->sepc=regs->sepc+4;
            child_regs->sp=task[i]->thread.sp;
            child_regs->x2=task[i]->thread.sp;
            child_regs->a0=0;   //返回pid

            task[i]->pgd=(unsigned long*)kalloc();
            for(int j=0;j<512;j++)
                task[i]->pgd[j]=swapper_pg_dir[j];
            //用户栈
            unsigned long u_mode_stack=kalloc();
            create_mapping(task[i]->pgd,USER_END-PGSIZE,u_mode_stack-PA2VA_OFFSET,PGSIZE,0x1f);
            uint64 parent_user_stack=regs->sscratch;
            char* child_stack=(char*)(u_mode_stack);
            char* parent_stack=(char*)(((parent_user_stack)>>12<<12));
            for(int j=0;j<PGSIZE;j++){
                child_stack[j]=parent_stack[j];
            }
            for(int j=0;j<current->vma_cnt;j++){
                struct vm_area_struct* vma=&(current->vmas[j]);
                uint64 vm_start=vma->vm_start,vm_end=vma->vm_end;
                for(int k=0;k<vma->vm_content_size_in_file;k+=PGSIZE){
                    //uint64 va=(vma->vm_content_offset_in_file+(uint64)uapp_start+k);
                    uint64 va=(vm_start+k);
                    uint64 index1 = ((va)&0x7fc0000000)>>30;
                    uint64 index2 = ((va)&0x3fe00000)>>21;
                    uint64 index3 = ((va)&0x1ff000)>>12;
                    pagetable_t pgd=(uint64*)((uint64_t)current->pgd+PA2VA_OFFSET);
                    if(!(pgd[index1]&1)){
                        continue;
                    }
                    uint64* page2=((pgd[index1]>>10)<<12)+PA2VA_OFFSET;
                    if(!(page2[index2]&1)){
                        continue;
                    }
                    uint64* page3 = ((page2[index2]>>10)<<12)+PA2VA_OFFSET;
                    if(!(page3[index3]&1)){
                        continue;
                    }
                    uint64 copy=kalloc();
                    create_mapping(task[i]->pgd,(va)>>12<<12,copy-PA2VA_OFFSET,PGSIZE,(vma->vm_flags)|0x11);
                    char* src=(char*)((vma->vm_content_offset_in_file+(uint64)uapp_start+k)>>12<<12);
                    char* dist=(char*)copy;
                    for(int l=0;l<PGSIZE;l++){
                        dist[l]=src[l];
                    }
                }
            }
            task[i]->pgd=(unsigned long)task[i]->pgd-PA2VA_OFFSET;
            
            return i;
        }
    }
    
    return -1;

}


void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    //time interrupt
    if((scause&0x8000000000000000)!=0){
        if((scause&0x7fffffffffffffff)==5){
            printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
            //printk("int\n");
        }
    }
    else{
        //environmental call from U mode
        if(scause==8){
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
            else if (regs->a7 == SYS_CLONE) {
                regs->a0=sys_clone(regs);
                regs->sepc+=4;
                //while(1);
            } 
            else {
                printk("[S] Unhandled syscall: %lx", scause);
                while (1);
            }
        }

            /*
            1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
            2. 通过 find_vma() 查找 Bad Address 是否在某个 vma 中
            3. 分配一个页，将这个页映射到对应的用户地址空间
            4. 通过 (vma->vm_flags | VM_ANONYM) 获得当前的 VMA 是否是匿名空间
            5. 根据 VMA 匿名与否决定将新的页清零或是拷贝 uapp 中的内容
            */
        else if(scause==12||scause==13||scause==15){
            printk("[S] scause: %lx",scause);
            if(scause==13||scause==15){
                printk(", stval:%lx,sepc:%lx",regs->stval,regs->sepc);
            }
            printk("\n");
            do_page_fault(regs);
        }

        else{
            printk("[S] Unhandled trap, ");
            printk("scause: %lx, ", scause);
            printk("stval: %lx, ", regs->stval);
            printk("sepc: %lx\n", regs->sepc);
            while (1);
        }
   
    }
    

}
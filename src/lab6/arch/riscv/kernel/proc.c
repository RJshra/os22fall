#include "proc.h"
#include "mm.h"
#include "rand.h"
#include "defs.h"
#include "elf.h"
#include "printk.h"
#include "string.h"
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);


struct task_struct* idle;           // idle process
        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
struct task_struct* current;

extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern char uapp_start[];
extern char uapp_end[];

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file){
        struct vm_area_struct* vma=&(task->vmas[task->vma_cnt++]);
        vma->vm_start=addr;
        vma->vm_end=addr+length;
        vma->vm_flags=flags;
        vma->vm_content_offset_in_file=vm_content_offset_in_file;
        vma->vm_content_size_in_file=vm_content_size_in_file;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr){
    for(int i=0;i<task->vma_cnt;i++){
        if(task->vmas[i].vm_start<=addr&&task->vmas[i].vm_end>=addr)
            return &(task->vmas[i]);
    }
    return NULL;
}

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)uapp_start;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            //代码和数据区域：
            //该区域从 ELF 给出的 Segment 起始地址 phdr->p_offset 开始
            do_mmap(task,phdr->p_vaddr,phdr->p_memsz,((phdr->p_flags)<<1),
                phdr->p_offset,phdr->p_filesz);
        }
    }

    //用户栈：范围为 [USER_END - PGSIZE, USER_END) ，权限为 VM_READ | VM_WRITE, 并且是匿名的区域
    do_mmap(task,USER_END-PGSIZE,PGSIZE,VM_R_MASK|VM_W_MASK|VM_ANONYM,0,0);
    
    task->thread.sepc = ehdr->e_entry; // the program starting address
    task->thread.sscratch=USER_END;
    //sstatus
    //spp[8]=0   spie[5]=1  sum[18]=1
    task->thread.sstatus=0x40020;
}

void task_init() {

    idle=(struct task_struct*)kalloc();
    idle->state=TASK_RUNNING;
    idle->counter=0;
    idle->priority=1;
    idle->pid=0;
    current=idle;
    task[0]=idle;

    for(int i=1;i<2;i++){
        task[i]=(struct task_struct*)kalloc();
        task[i]->state=TASK_RUNNING;
        task[i]->counter=0;
        task[i]->priority=rand();
        task[i]->pid=i;
        task[i]->thread.ra=(uint64)(&__dummy);
        task[i]->thread.sp=(uint64)task[i]+PGSIZE;
        
        //task[i]->pgd=(unsigned long*)kalloc();
        task[i]->pgd=(unsigned long*)kalloc();
        for(int j=0;j<512;j++){
           task[i]->pgd[j]=swapper_pg_dir[j];
        }
        load_program(task[i]);

        task[i]->pgd=(unsigned long)task[i]->pgd-PA2VA_OFFSET;
    
    }
    for(int i=2;i<NR_TASKS;i++){
        task[i]=NULL;
    }
    printk("...proc_init done!\n");
}

//thread function
void dummy() {
    
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            //printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running. thread space begin at %lx\n", current->pid,current);
        }
    }
}

#ifdef SJF
void schedule(void) {
    int index=0;
    int flag=0;
    for(int i=1;i<NR_TASKS;i++){
        if(task[i]==NULL)
            continue;
        if(task[i]->state!=TASK_RUNNING)
            continue;   //判断线程状态
        flag=1;
        if(task[i]->counter==0)continue;
        if((task[index]->counter==0)||(task[i]->counter<task[index]->counter))
            index=i;
    }
    //如果所有counter都是0并且状态是TASK_RUNNING就重新设置counter
    if((index==0)&&flag){
        index=0;
        for(int i=1;i<NR_TASKS;i++){
            if(task[i]==NULL||task[i]->state!=TASK_RUNNING)
                continue;
            task[i]->counter=rand();
            //printk("[S] SET [PID = %d COUNTER = %d]\n",i,task[i]->counter);
        }

        schedule();
    }
    else{
        switch_to(task[index]);
    }
}
#endif

#ifdef PRIORITY
void schedule(void) {
    int index=0;
    int flag=0;
    for(int i=1;i<NR_TASKS;i++){
        if(task[i]==NULL||task[i]->state!=TASK_RUNNING)continue;
        flag=1;
        if(task[i]->counter==0)continue;
        if((index==0)||(task[i]->priority>task[index]->priority)||(task[i]->priority==task[index]->priority&&task[i]->counter<task[index]->counter))
            index=i;
    }
    if(index==0&&flag){
        index=0;
        for(int i=1;i<NR_TASKS;i++){
            if(task[i]==NULL||task[i]->state!=TASK_RUNNING)continue;
            task[i]->counter=rand();
           // printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",i,task[i]->priority,task[i]->counter);
        }

        schedule();
    }
    else{
        switch_to(task[index]);
    }
}
#endif

void switch_to(struct task_struct* next) {
    if(current==next||next->pid==0)
        return;
#ifdef SJF
    printk("[S] switch to [PID = %d COUNTER = %d]\n",next->pid,next->counter);
#endif

#ifdef PRIORITY
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",next->pid,next->priority,next->counter);
#endif
   // next->counter--;
    struct task_struct* temp=current;
    current=next;
    __switch_to(temp,next);
}


void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
    if(current->pid==0)
        schedule();
    else{
        current->counter--;
        if(current->counter>0)
            return;
        
        schedule();
    }
    
}

#include "proc.h"
#include "mm.h"
#include "rand.h"
#include "defs.h"
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
struct task_struct* current;
void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    idle=(struct task_struct*)kalloc();
    idle->state=TASK_RUNNING;
    idle->counter=0;
    idle->priority=1;
    idle->pid=0;
    current=idle;
    task[0]=idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for(int i=1;i<NR_TASKS;i++){
        task[i]=(struct task_struct*)kalloc();
        task[i]->state=TASK_RUNNING;
        task[i]->counter=0;
        task[i]->priority=rand();
        task[i]->pid=i;
        task[i]->thread.ra=&(__dummy);
        task[i]->thread.sp=(uint64)task[i]+PGSIZE;
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
        if(task[i]->state!=TASK_RUNNING)continue;   //判断线程状态
        flag=1;
        if(task[i]->counter==0)continue;
        if((task[index]->counter==0)||(task[i]->counter<task[index]->counter))
            index=i;
    }
    //如果所有counter都是0并且状态是TASK_RUNNING就重新设置counter
    if((index==0)&&flag){
        index=0;
        for(int i=1;i<NR_TASKS;i++){
            if(task[i]->state!=TASK_RUNNING)
                continue;
            task[i]->counter=rand();
            printk("SET [PID = %d COUNTER = %d]\n",i,task[i]->counter);
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
        if(task[i]->state!=TASK_RUNNING)continue;
        flag=1;
        if(task[i]->counter==0)continue;
        if((index==0)||(task[i]->priority>task[index]->priority)||(task[i]->priority==task[index]->priority&&task[i]->counter<task[index]->counter))
            index=i;
    }
    if(index==0&&flag){
        index=0;
        for(int i=1;i<NR_TASKS;i++){
            if(task[i]->state!=TASK_RUNNING)continue;
            task[i]->counter=rand();
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",i,task[i]->priority,task[i]->counter);
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
    printk("switch to [PID = %d COUNTER = %d]\n",next->pid,next->counter);
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

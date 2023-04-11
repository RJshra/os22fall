// arch/riscv/kernel/vm.c
#include "defs.h"
#include "types.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    memset(early_pgtbl, 0, PGSIZE);
    //要把va的地址映射到pa上
    unsigned long va = 0x80000000;
    unsigned long pa = va;
    //先找到中间9位对应到数组的index
    uint64 index = (va&0x0000007fc0000000)>>30;
    //ppn=pa>>12,pte中ppn在第10位，<<10
    unsigned long entry = (pa>>12)<<10;
    //后4位置1
    entry|=0xF;
    early_pgtbl[index]=entry;
    
    //PA + PV2VA_entry == VA
    va = 0xffffffe000000000;
    index = (va&0x0000007fc0000000)>>30;
    entry = (pa>>12)<<10;
    entry|=0xF;
    early_pgtbl[index]=entry;

    return;
}


/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern uint64 _stext,_etext,_srodata,_erodata,_sbss,_sdata,_ekernel;

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);
    // No OpenSBI mapping required
    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir,(uint64)(&_stext),(uint64)(&_stext)-PA2VA_OFFSET,(uint64)(&_etext)-(uint64)(&_stext),11);
    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir,(uint64)(&_srodata),(uint64)(&_srodata)-PA2VA_OFFSET,(uint64)(&_erodata)-(uint64)(&_srodata),3);
    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir,(uint64)(&_sdata),(uint64)(&_sdata)-PA2VA_OFFSET,(uint64)(PHY_END+PA2VA_OFFSET)-(uint64)(&_sdata),7);
    // set satp with swapper_pg_dir
    uint64 dir = (uint64)swapper_pg_dir-PA2VA_OFFSET;
    //YOUR CODE HERE
    __asm__ volatile (
			"add t0,x0, %[dir]\n"
			"addi t1, x0, 0x8\n"
			"slli t1,t1,60\n"
			// "li t2, 0xFFFFFFDF80000000\n"
			// "sub t0, t0, t2\n"
			"srli t0,t0, 12\n"
			"add t1, t1, t0\n"
			"csrw satp, t1\n"
			:: [dir]"r"(dir)
            :"memory"
   		 );
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    // flush icache
    asm volatile("fence.i");
    return;
}


/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */  
    //不再进行等值映射，映射到高地址 PA + PV2VA_entry == VA
    uint64 have_allocate = 0;
    //printk("%d\n",sz);
    while(sz>have_allocate){
        //页表存放pte，根页表是vpn[2]
        uint64* page2;
        uint64 index1 = (va&0x0000007fc0000000)>>30;
        if(pgtbl[index1]&1){
            //do nothing
        }
        else{
            page2 = (uint64*)kalloc();
            uint64 entry1 = ((uint64)page2-PA2VA_OFFSET)>>12<<10;
            entry1+=1;
            pgtbl[index1]=entry1;
        }
        uint64* page3;
        uint64 index2 = (va&0x000000003fe00000)>>21;
        if(page2[index2]&1){
            //
        }
        else{
            page3 = (uint64*)kalloc();
            uint64 entry2 = ((uint64)page3-PA2VA_OFFSET)>>12<<10;
            entry2+=1;
            page2[index2]=entry2;
        }
        uint64 index3 = (va&0x00000000001ff000)>>12;
        uint64 entry3 = pa>>12<<10;
        entry3+=perm;
        page3[index3] = entry3;

        va +=  PGSIZE;
        pa +=  PGSIZE;
        have_allocate+=PGSIZE;

       // printk("root %d\nmiddle  %d\n  leaf  %d\n",pgtbl,page2,page3);
    }
   
}   

#include "printk.h"
#include "sbi.h"
#include"defs.h"
extern void test();

int start_kernel() {
    printk("hello riscv\n");
    test(); // DO NOT DELETE !!!

	return 0;
}

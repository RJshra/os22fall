#include "print.h"
#include "sbi.h"
#include"defs.h"
extern void test();

int start_kernel() {
    puts(" Hello RISC-V\n");
    puti(2022);
    test(); // DO NOT DELETE !!!

	return 0;
}

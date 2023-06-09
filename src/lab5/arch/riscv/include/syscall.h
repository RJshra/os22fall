#define SYS_WRITE   64
#define SYS_GETPID  172
long sys_write(unsigned int fd, const char* buf, int count);
long sys_getpid();
struct pt_regs{
    unsigned long x0;
    unsigned long ra;
    unsigned long x2;
    unsigned long gp;
    unsigned long tp;
    unsigned long t0;
    unsigned long t1;
    unsigned long t2;
    unsigned long s0;
    unsigned long s1;
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
    unsigned long a4;
    unsigned long a5;
    unsigned long a6;
    unsigned long a7;
    unsigned long s2;
    unsigned long s3;
    unsigned long s4;
    unsigned long s5;
    unsigned long s6;
    unsigned long s7;
    unsigned long s8;
    unsigned long s9;
    unsigned long s10;
    unsigned long s11;
    unsigned long t3;
    unsigned long t4;
    unsigned long t5;
    unsigned long t6;
    unsigned long sp;
    unsigned long sstatus;
    unsigned long sepc;
    unsigned long scause;
};

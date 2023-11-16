
#include "sysinfo.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"



uint64 sysinfo(uint64 info) {
    struct proc *p = myproc();
    struct sysinfo systemInfo;
    systemInfo.freemem = count_free_bytes();
    systemInfo.nproc = count_proc();
    return copyout(p->pagetable,  info, (char*)&systemInfo, sizeof(struct sysinfo));;
}
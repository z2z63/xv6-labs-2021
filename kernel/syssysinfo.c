#include "sysinfo.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_sysinfo(void){
    uint64 info; // 用户空间的指针
    if(argaddr(0, &info)!=0){
        return -1;
    }
    return sysinfo(info);
}
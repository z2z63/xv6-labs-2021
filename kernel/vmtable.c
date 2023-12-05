#include "types.h"
#include "spinlock.h"
#include "param.h"
#include "riscv.h"
#include "proc.h"

void vmt_free(struct vmt *v) {
    v->length = 0;
    v->file = 0;
}

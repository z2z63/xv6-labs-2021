// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock[NCPU];
    struct run *freelist[NCPU];
} kmem;

void
kinit() {
    initlock(&kmem.lock[0], "kmem-0");
    initlock(&kmem.lock[1], "kmem-1");
    initlock(&kmem.lock[2], "kmem-2");
    initlock(&kmem.lock[3], "kmem-3");
    initlock(&kmem.lock[4], "kmem-4");
    initlock(&kmem.lock[5], "kmem-5");
    initlock(&kmem.lock[6], "kmem-6");
    initlock(&kmem.lock[7], "kmem-7");
    freerange(end, (void *) PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char *) PGROUNDUP((uint64) pa_start);
    int i = 0;
    for (; p + PGSIZE <= (char *) pa_end; p += PGSIZE){
        memset(p, 1, PGSIZE);
        struct run* r = (struct run*)p;
        acquire(&kmem.lock[i]);
        r->next = kmem.freelist[i];
        kmem.freelist[i] = r;
        release(&kmem.lock[i]);
        i = (i + 1) % NCPU;
    }

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa) {
    struct run *r;

    if (((uint64) pa % PGSIZE) != 0 || (char *) pa < end || (uint64) pa >= PHYSTOP)
        panic("kfree");

    int id;
    if (intr_get()) {     // 如果开启了中断，就关闭。因为接下来要使用这个CPU独占的资源，不能被打断然后切换到其他CPU
        intr_off();
        id = cpuid();
        intr_on();
    }else{
        id = cpuid();   // 只要知道这个页应该还给哪个CPU，归还这个操作即使是其他CPU完成的也没影响
    }

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *) pa;

    acquire(&kmem.lock[id]);
    r->next = kmem.freelist[id];
    kmem.freelist[id] = r;
    release(&kmem.lock[id]);


}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void) {
    int id;
    if (intr_get()) {     // 如果开启了中断，就关闭。因为接下来要使用这个CPU独占的资源，不能被打断然后切换到其他CPU
        intr_off();
        id = cpuid();
        intr_on();
    }else{
        id = cpuid();   // 即使后面切换到了其他CPU也没关系。
    }

    struct run *r;
    acquire(&kmem.lock[id]);
    r = kmem.freelist[id];
    if (r) {
        kmem.freelist[id] = r->next;
        release(&kmem.lock[id]);
    } else {
        release(&kmem.lock[id]);
        for (int i = 0; i < NCPU; i++) {    // 偷其他CPU的空闲页
            if (i == id) {  // 不偷自己的
                continue;
            }
            acquire(&kmem.lock[i]);     // 上锁，因为偷其他CPU的空闲页，不能让这个CPU同时使用空闲页
            r = kmem.freelist[i];
            if (r) {    // 偷到了
                kmem.freelist[i] = r->next;
                release(&kmem.lock[i]);
                break;
            }
            // 没偷到，释放这个CPU的锁，偷下一个CPU的空闲页
            release(&kmem.lock[i]);
        }
    }

    if (r)
        memset((char *) r, 5, PGSIZE); // fill with junk
    return (void *) r;
}

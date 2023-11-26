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
static uint16 *get_pa_ref(void *pa);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

uint16 page_ref[32*1024];
struct spinlock page_ref_lock;

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;

uint8 first_run = 1;

void
kinit() {
    initlock(&kmem.lock, "kmem");
    initlock(&page_ref_lock, "page_ref_lock");
    freerange(end, (void *) PHYSTOP);
    memset(page_ref, 0, sizeof(page_ref));     // 初始化所有页面的引用计数为0
    first_run = 0;      // 引用计数机制已经建立
}

void
freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char *) PGROUNDUP((uint64) pa_start);
    for (; p + PGSIZE <= (char *) pa_end; p += PGSIZE)
        kfree(p);
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

    if (!first_run) {     // 在初始化时,引用计数机制还没有建立，直接释放页面
        acquire(&page_ref_lock);
        (*get_pa_ref(pa))--;
        if (*get_pa_ref(pa) > 0) {      // 只有当页面的引用计数降为0时，才释放页面
            release(&page_ref_lock);
            return;
        } else {
            release(&page_ref_lock);
        }
    }

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *) pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

    if (r)
        memset((char *) r, 5, PGSIZE); // fill with junk
    if(!first_run && r != 0){     // 引用计数机制建立后，才能使用引用计数机制
        acquire(&page_ref_lock);
        *get_pa_ref((void *)r) = 1;
        release(&page_ref_lock);
    }

    return (void *) r;
}

static uint16 *get_pa_ref(void *pa) {
    // 物理内存128M，最多有128M/4KB=32K个页面
    // 每个页面使用2B记录引用计数，所以需要64K的内存
    // 保存64KB的记录需要16个页面
    uint64 index = ((uint64) pa - KERNBASE) >> PGSHIFT;
    if(index >= sizeof(page_ref)/sizeof(page_ref[0])){
        panic("get_pa_ref: index out of range");
    }
    return &page_ref[index];
}

void inc_page_ref(void *pa){
    acquire(&page_ref_lock);
    (*get_pa_ref(pa))++;
    release(&page_ref_lock);
}
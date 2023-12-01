// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

//struct {
//  struct spinlock lock;
//  struct buf buf[NBUF];
//
//  // Linked list of all buffers, through prev/next.
//  // Sorted by how recently the buffer was used.
//  // head.next is most recent, head.prev is least.
//  struct buf head;
//} bcache;

struct entry {
    uint key;
    struct buf value;
    struct entry *next;
};

#define NBUCKET 53

struct {
    struct entry *table[NBUCKET];
    struct spinlock bucket_lock[NBUCKET];
    struct entry backends[NBUF];
} bcache;

char lock_names[NBUCKET][16];

uint64 hashcode1(uint dev, uint blockno) {
    return (/*(dev << 27) +*/ blockno) % NBUCKET;
}

uint64 hashcode(struct buf *b) {
    return hashcode1(b->dev, b->blockno);
}


void
binit(void) {
    for (int i = 0; i < NBUCKET; ++i) {
        snprintf(lock_names[i], sizeof(lock_names[i]), "bcache-%d", i);// 锁的粒度是槽
        initlock(&bcache.bucket_lock[i], lock_names[i]);
    }

    // 将backend中的所有entry初始化然后插入到table中
    for (int i = 0; i < NBUF; ++i) {
        struct buf *b = &bcache.backends[i].value;
        b->dev = -1;
        b->blockno = i;
        b->valid = 0;
        b->refcnt = 0;
        b->ticks = 0;
        uint64 hash = hashcode(b);
        bcache.backends[i].key = hash;

        acquire(&bcache.bucket_lock[hash]);
        bcache.backends[i].next = bcache.table[hash];
        bcache.table[hash] = &bcache.backends[i];
        release(&bcache.bucket_lock[hash]);
    }
}

/**
 * 找到了LRU的entry后，这个entry可能又被使用，所以需要上锁
 * */
struct entry* find_lru_entry(uint64 me){
    struct entry* lru_entry = 0;
    uint64 min = -1;

    acquire(&bcache.bucket_lock[me]);   // 优先从自己的bucket中找，此时不使用LRU策略
    for (struct entry *p = bcache.table[me]; p != 0; p = p->next) {
        if (p->value.refcnt == 0 && p->value.ticks < min) {
            lru_entry = p;
            min = p->value.ticks;
        }
    }
    if(lru_entry != 0){
        goto found;
    }
    release(&bcache.bucket_lock[me]);


    struct spinlock* last_lock = 0;     // 始终持有上一个找到的最久未使用的entry的锁
    char flag = 0;
    for (int i = 0; i < NBUCKET; ++i) {
        if(i == me){
            continue;
        }
        acquire(&bcache.bucket_lock[i]);
        for (struct entry *p = bcache.table[i]; p != 0; p = p->next) {
            if (p->value.refcnt == 0 && p->value.ticks < min) {
                min = p->value.ticks;
                lru_entry = p;
                flag = 1;
            }
        }
        if(flag){
            flag = 0;
            if(last_lock !=0){
                release(last_lock);
            }
            last_lock = &bcache.bucket_lock[i];
        }else{
            release(&bcache.bucket_lock[i]);
        }
    }

    found:
    // 从bucket中删除entry
        if(bcache.table[hashcode(&lru_entry->value)] == lru_entry) {
            bcache.table[hashcode(&lru_entry->value)] = lru_entry->next;
        }else{
            for (struct entry *p = bcache.table[hashcode(&lru_entry->value)]; p != 0; p = p->next) {
                if(p->next == lru_entry){
                    p->next = lru_entry->next;
                    break;
                }
            }
        }

        return lru_entry;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// Invariants:
// 1. 所有entry都在哈希表中，（entry的转移会暂时破坏）
// 2. 所有entry的blockno都是唯一的，不存在重复（entry的转移会暂时破坏）
static struct buf *
bget(uint dev, uint blockno) {
    struct buf *b;

    uint64 hash = hashcode1(dev, blockno);
    acquire(&bcache.bucket_lock[hash]);
    // hit
    for (struct entry *p = bcache.table[hash]; p != 0; p = p->next) {
        if (p->value.dev == dev && p->value.blockno == blockno) {
            p->value.refcnt++;

            acquire(&tickslock);
            p->value.ticks = ticks;        // 更新上次使用时间
            release(&tickslock);

            release(&bcache.bucket_lock[hash]);
            acquiresleep(&p->value.lock);
            return &p->value;
        }
    }
    release(&bcache.bucket_lock[hash]);

    // miss
    struct entry *new_entry = find_lru_entry(hash);
    uint64 oldhash = hashcode(&new_entry->value);
    b = &new_entry->value;
    b->dev = dev;
    b->blockno = blockno;
    b->refcnt = 1;
    b->valid = 0;

    uint64 newhash = hashcode(b);
    new_entry->key = newhash;
    if(oldhash == newhash){
        new_entry->next = bcache.table[newhash];
        bcache.table[newhash] = new_entry;
    }else{
        acquire(&bcache.bucket_lock[newhash]);
        new_entry->next = bcache.table[newhash];
        bcache.table[newhash] = new_entry;
        release(&bcache.bucket_lock[newhash]);
    }

    release(&bcache.bucket_lock[oldhash]);
    acquiresleep(&b->lock);
    return b;

//    panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno) {
    struct buf *b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);
    acquire(&bcache.bucket_lock[hashcode(b)]);
    b->refcnt--;
    if(b->refcnt==0){
        b->ticks = ticks;
    }
    release(&bcache.bucket_lock[hashcode(b)]);
}

void
bpin(struct buf *b) {
    acquire(&bcache.bucket_lock[hashcode(b)]);
    b->refcnt++;
    release(&bcache.bucket_lock[hashcode(b)]);
}

void
bunpin(struct buf *b) {
    acquire(&bcache.bucket_lock[hashcode(b)]);
    b->refcnt--;
    release(&bcache.bucket_lock[hashcode(b)]);
}



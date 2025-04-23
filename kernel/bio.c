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

#define BIO_HASHMODS 13

struct {
    struct spinlock lock[BIO_HASHMODS];
    struct buf buf[NBUF];
    struct buf hasht[BIO_HASHMODS];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    //   struct buf head;

    // Do not use THIS!!!
    // struct buf unused;
    // struct spinlock unused_lock;
} bcache;

void binit(void) {
    struct buf *b;
    // struct spinlock *l;
    // l = bcache.lock;
    // for (l = bcache.lock; l < bcache.lock + BIO_HASHMODS; l++) {
    //     initlock(l, "bcache");
    // }
    for (int i=0; i<BIO_HASHMODS; i++) {
        initlock(&bcache.lock[i], "bcache");
    }
    for (b = bcache.hasht; b < bcache.hasht + BIO_HASHMODS; b++) {
        b->prev = b->next = b;
    }

    // initlock(&bcache.unused_lock, "bcache_u");

    // Create linked list of buffers
    //   bcache.head.prev = &bcache.head;
    //   bcache.head.next = &bcache.head;
    // bcache.unused.prev = bcache.unused.next = &bcache.unused;
    int i=0;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        // b->next = bcache.head.next;
        // b->prev = &bcache.head;
        initsleeplock(&b->lock, "buffer");
        b->prev = &bcache.hasht[i];
        b->next = bcache.hasht[i].next;
        b->prev->next = b->next->prev = b;
        i = (i+1)%BIO_HASHMODS;
        // b->next = bcache.unused.next;
        // b->prev = &bcache.unused;
        // bcache.unused.next->prev = b;
        // bcache.unused.next = b;
        // bcache.head.next->prev = b;
        // bcache.head.next = b;
    }
}

static int hash(uint dev, uint blockno) {
    static const uint64 magic_number_1 = 10001;
    // printf("%d, %d\n", dev, blockno);
    return ((dev * magic_number_1) + blockno)%BIO_HASHMODS;
}

// Acquire two locks
static void acquire_two_locks(int a, int b) {
    if (a < b) {
        acquire(&bcache.lock[a]);
        acquire(&bcache.lock[b]);
    } else if (a > b) {
        acquire(&bcache.lock[b]);
        acquire(&bcache.lock[a]);
    } else {
        acquire(&bcache.lock[b]);
    }
}

// Release two locks
static void release_two_locks(int a, int b) {
    if (a < b) {
        release(&bcache.lock[a]);
        release(&bcache.lock[b]);
    } else if (a > b) {
        release(&bcache.lock[b]);
        release(&bcache.lock[a]);
    } else {
        release(&bcache.lock[b]);
    }
}

// Try to do bget at pos, 
// when block is not cached in pos, allocate from a free block in target
static struct buf *bget_try(uint dev, uint blockno, int pos, int target) {
    struct buf *b;

    acquire_two_locks(pos, target);

    // Check pos in case that (dev, blockno) has been added.
    for (b = bcache.hasht[pos].next; b != &bcache.hasht[pos]; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release_two_locks(pos, target);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Check target
    for (b = bcache.hasht[target].next; b != &bcache.hasht[target]; b = b->next) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;

            // Remove from bucket target
            b->next->prev = b->prev;
            b->prev->next = b->next;

            // Insert into bucket pos
            b->prev = &bcache.hasht[pos];
            b->next = bcache.hasht[pos].next;
            b->prev->next = b->next->prev = b;

            release_two_locks(pos, target);
            acquiresleep(&b->lock);
            return b;
        }
    }

    release_two_locks(pos, target);

    return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno) {
    struct buf *b;

    int pos = hash(dev, blockno);

    b = 0;
    // Keep trying get a cached block or a free block
    for (int target=pos; !b; target=(target+1)%BIO_HASHMODS) {
        b = bget_try(dev, blockno, pos, target);
    }
    return b;

    panic("bget: no buffers");
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
void bwrite(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    int pos = hash(b->dev, b->blockno);
    acquire(&bcache.lock[pos]);
    b->refcnt--;
    // You don't care about it
    // if (b->refcnt == 0) {
    //     // no one is waiting for it.
    //     // b->next->prev = b->prev;
    //     // b->prev->next = b->next;
    //     // b->next = bcache.head.next;
    //     // b->prev = &bcache.head;
    //     // bcache.head.next->prev = b;
    //     // bcache.head.next = b;

    //     // Remove from hasht
    //     // b->prev->next = b->next;
    //     // b->next->prev = b->prev;

    //     // release(&bcache.lock[pos]);

    //     // Insert to unused list
    //     // acquire(&bcache.unused_lock);
    //     // b->prev = &bcache.unused;
    //     // b->next = bcache.unused.next;
    //     // b->next->prev = b->prev->next = b;
    //     // release(&bcache.unused_lock);
    // }
    release(&bcache.lock[pos]);

}

void bpin(struct buf *b) {
    int pos = hash(b->dev, b->blockno);
    acquire(&bcache.lock[pos]);
    b->refcnt++;
    release(&bcache.lock[pos]);
}

void bunpin(struct buf *b) {
    int pos = hash(b->dev, b->blockno);
    acquire(&bcache.lock[pos]);
    b->refcnt--;
    release(&bcache.lock[pos]);
}

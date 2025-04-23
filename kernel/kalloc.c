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
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  for (int i=0; i<NCPU; i++) {
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int cpu_id = cpuid();
  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void) {
    struct run *r;

    // Try to use the freelist on this cpu
    push_off();  // Ban the interruption!
    int cpu_id = cpuid();
    acquire(&kmem[cpu_id].lock);
    r = kmem[cpu_id].freelist;
    if (r) {
        kmem[cpu_id].freelist = r->next;
    }
    release(&kmem[cpu_id].lock);

    // Steal from other cpus
    for (int i = (cpu_id+1)%NCPU; !r && i != cpu_id; i=(i+1)%NCPU) {
        acquire(&kmem[i].lock);
        r = kmem[i].freelist;
        if (r) {
            kmem[i].freelist = r->next;
        }
        release(&kmem[i].lock);
    }
    pop_off();

    if (r)
        memset((char *)r, 5, PGSIZE);  // fill with junk
    return (void *)r;
}

// Calculate the total amount of free memory in bytes (by pages, running through freelist).
uint64 calc_freemem(void) {
    struct run *r;
    uint64 freemem = 0;

    for (int i = 0; i < NCPU; i++) {
        acquire(&kmem[i].lock);
        for (r = kmem[i].freelist; r; r = r->next) {
            freemem += PGSIZE;
        }
        release(&kmem[i].lock);
    }

    return freemem;
}

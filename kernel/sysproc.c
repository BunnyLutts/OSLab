#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
#ifdef LAB_TRAPS
  backtrace();
#endif
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
    uint64 start_addr;
    int len;
    uint64 ret_buf;
    argaddr(0, &start_addr);
    argint(1, &len);
    argaddr(2, &ret_buf);

    char *buf = kalloc();
    struct proc *p = myproc();
    if (len > PGSIZE * 8 || p == 0) {
        kfree(buf);
        return -1;
    }
    for (int i = 0; i < len; i++) {
        if (i%8==0) {
            buf[i/8] = 0;
        }
        uint64 addr = start_addr + i * PGSIZE;
        pte_t *pte = walk(p->pagetable, addr, 0);
        int bit = 0;
        if (pte != 0 && (*pte & PTE_A)) {
            bit = 1;
            *pte ^= PTE_A;
        }
        buf[i/8] |= (bit << (i%8));
    }

    if (copyout(p->pagetable, ret_buf, (char *)buf, (len/8 + (len%8 != 0))) < 0) {
        kfree(buf);
        return -1;
    }
    kfree(buf);

    return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// Set the trace mask for the current process.
uint64 sys_trace(void) {
    int mask;
    argint(0, &mask);
    struct proc *p = myproc();
    if (p) {
        p->trace_mask = (uint32)mask;
        return 0;
    } else return -1;
}

uint64 calc_freemem(void);
uint64 count_processes(void);

// Get the sysinfo struct for the current process.
uint64 sys_sysinfo(void) {
    uint64 addr;
    argaddr(0, &addr);
    struct sysinfo info;
    info.freemem = calc_freemem();
    info.nproc = count_processes();

    struct proc *p = myproc();
    if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0) {
        return -1;
    }
    return 0;
}

// Set alarm for the current process.
uint64 sys_sigalarm(void) {
    struct proc *p = myproc();
    int ticks;
    uint64 handler_addr;
    argint(0, &ticks);
    argaddr(1, &handler_addr);
    p->alarm_ticks = ticks;
    p->alarm_handler = handler_addr;
    p->cur_ticks = 0;
    return 0;
}

// Return from a signal handler.
uint64 sys_sigreturn(void) {
    struct proc *p = myproc();
    *p->trapframe = *p->trapfram2; // Restore the old trapframe.
    p->handler_id = 0; // Mark the handler as inactive.
    return p->trapframe->a0; // Restore the return value.
    // return 0;
}
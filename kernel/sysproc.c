#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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
  // lab pgtbl: your code here.
  // read arguments
  uint64 start_va;
  int page_nums;
  uint64 ubuffer_addr;
  argaddr(0, &start_va);
  argint(1, &page_nums);
  argaddr(2, &ubuffer_addr);

  if (page_nums > 64) {
    printf("page_nums: %d\n", page_nums);
    printf("sys_pgaccess: to many pages to check\n");
    return -1;
  }

  uint64 bitmap = 0;
  uint64 va = start_va;
  pagetable_t pagetable = myproc()->pagetable;
  for (int i = 0; i < page_nums; i++) {
    pte_t *pte = walk(pagetable, va, 0);
    if (pte == 0) {
      printf("sys_pgaccess: invalid page\n");
      return -1;
    }
    if (PTE_FLAGS(*pte) & PTE_A) {
      bitmap |= (1L << i);
    }

    // clear PTE_A bit
    *pte &= ~PTE_A;

    va += PGSIZE;
  }
  if (copyout(pagetable, ubuffer_addr, (char *)&bitmap, sizeof(bitmap)) < 0) {
    printf("sys_pgaccess: invalid buffer\n");
    return -1;
  }

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

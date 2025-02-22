#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
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
  if(n < 0)
    n = 0;
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

uint64 
sys_getppid(void){ 
  return myproc()->parent->pid;
} 

uint64 
sys_getcpids(void){ 
  struct file *f;
  int p;
  uint64 n;

  argaddr(0, &p);
  argint(1, &n);

  return 0; // temp
} 

/*The system call for getcpid is int getcpids(int *cpids, int max); 
 
Here, argument cpids is a pointer to an array of integers and 
argument max is the number of elements in the array. Before 
making this system call, the calling process should have set 
up the array.   
 
This system call returns the number (up to max) of child 
processes of the calling process; the ids of these child 
processes are returned in the array pointed to by argument 
cpids. In addition, argument max specifies the maximum number 
of the child processes which this system call need to return 
the information of.  
---------------------------------------------------------------
the system call for write is int write(int, const void*, int);
uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;
  
  argaddr(1, &p);
  argint(2, &n);
  if(argfd(0, 0, &f) < 0)
    return -1;

  return filewrite(f, p, n);
}

note: filewrite is int filewrite(struct file*, uint64, int n);
  
*/

uint64 
sys_getpaddr(void){ 
  uint64 vaddr;
  argaddr(0, &vaddr);
  int alloc = 0;

  // Get the page table
  pagetable_t pagetable = myproc()->pagetable;
  // Get the page table entry
  pte_t* pte = walk(pagetable, vaddr, alloc);

  if (pte != 0 && (*pte & PTE_V)) {
    // The PTE pointed to by bte is valid page entry
    int physical_address_of_va = PTE2PA(*pte) | (vaddr & 0xFFF);
    return physical_address_of_va;
  }
  else {
    // The PTE pointed to by pte is invalid; that is, the virtual address is invalid
    return -1;
  }
}

uint64 
sys_gettraphistory(void){ 
  int trapcount, syscallcount, devintcount, timerintcount;
  return 0;
} 


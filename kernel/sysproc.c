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
  // Get the parent process ID of the calling process and return it.
  return myproc()->parent->pid;
} 

uint64 
sys_getcpids(void){ 
  int max;
  uint64 cpids;
  struct proc *p = myproc();
  argaddr(0, &cpids);
  argint(1, &max);
  
  // Get the child processes of the calling process
  struct proc *child = proc;
  int count = 0;
  
  // Iterate through the process table to find child processes
  for (int i = 0; i < NPROC; i++) {
    // Check if the process is a child of the calling process
    if (child[i].parent == p) {
      if (count < max) {
        if (copyout(p->pagetable, cpids + count * sizeof(int), (char *)&child[i].pid, sizeof(int)) < 0) {
          return -1;
        }
        count++;
      } else {
        break;
      }
    }
  }
  
  return count; // Return the number of child processes copied

} 

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
  // copy the current process’ trapcount, syscallcount, devintcount and timerintcount to the addresses that are passed in to this system call as arguments.
  uint64 trapcount, syscallcount, devintcount, timerintcount;
  argaddr(0, &trapcount);
  argaddr(1, &syscallcount);
  argaddr(2, &devintcount);
  argaddr(3, &timerintcount);
  struct proc *p = myproc();
  
  // Copy the values from the process structure to the addresses passed in
  if (copyout(p->pagetable, trapcount, (char *)&p->trapcount, sizeof(p->trapcount)) < 0) {
    return -1;
  }
  if (copyout(p->pagetable, syscallcount, (char *)&p->syscallcount, sizeof(p->syscallcount)) < 0) {
    return -1;
  }
  if (copyout(p->pagetable, devintcount, (char *)&p->devintcount, sizeof(p->devintcount)) < 0) {
    return -1;
  }
  if (copyout(p->pagetable, timerintcount, (char *)&p->timerintcount, sizeof(p->timerintcount)) < 0) {
    return -1;
  }
  return 0;
} 

// 3.3.1
uint64 sys_nice(void){ 
  int new_nice;
  argint(0, &new_nice);
  struct proc *p = myproc();
  // If new_nice is an integer between –20 and 19,
  // the caller’s nice is set to new_nice.
  // Otherwise, the nice value is unchanged.
  if (new_nice >= -20 && new_nice <= 19) {
    p->nice = new_nice;
  }
  return p->nice; // Return the current nice value
}

// 3.3.2
// returns the caller’s actual runtime and virtual runtime to the integer variables
// pointed to by the arguments, respectively.
uint64 sys_getruntime(void){ 
  uint64 runtime, vruntime;
  argaddr(0, &runtime);
  argaddr(1, &vruntime);
  struct proc *p = myproc();

  // Copy the values from the process structure to the addresses passed in
  if (copyout(p->pagetable, runtime, (char *)&p->runtime, sizeof(p->runtime)) < 0) {
    return -1;
  }
  if (copyout(p->pagetable, vruntime, (char *)&p->vruntime, sizeof(p->vruntime)) < 0) {
    return -1;
  }
  return 0;
}

// 3.3.3
// Starts the share scheduler by setting the variable cfs to 1 in proc.c. It also set the
// parameters cfs_sched_latency, cfs_max_timeslice and cfs_min_timeslice to the arguments,
// respectively. It returns 1. 
uint64 sys_startcfs(void){ 
  int latency, max, min;
  argint(0, &latency);
  argint(1, &max);
  argint(2, &min);

  cfs = 1; // Start the CFS scheduler

  cfs_sched_latency = latency;
  cfs_max_timeslice = max;
  cfs_min_timeslice = min;
  
  return 1; // Return success
}

// 3.3.3
// stops the share scheduler by setting the variable cfs to 0 in proc.c. It returns 1. 
uint64 sys_stopcfs(void){ 
  cfs = 0; // Stop the CFS scheduler
  return 1; // Return success
}
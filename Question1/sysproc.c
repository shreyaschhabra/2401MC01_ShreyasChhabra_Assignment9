// sysproc.c — xv6 process-related system calls
// CHANGES FOR ASSIGNMENT 9:
//   Q1: Added sys_vmprint() — wraps vmprint() in vm.c
//   Q2: Modified sys_sbrk() for lazy allocation (only grows sz, no kalloc)
//   Q2: Added sys_getfaults() — returns proc->page_faults counter

#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

// ─── ASSIGNMENT 9, QUESTION 2: Lazy sbrk ─────────────────────────────────
// Original sbrk() called growproc() which immediately allocated pages.
// New behaviour: when n > 0, only advance proc->sz without allocating.
// When n < 0 (shrink), we still call deallocuvm() immediately.
int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  addr = myproc()->sz;

  if(n > 0){
    // LAZY: only bump the size, no physical allocation yet.
    // Physical frames will be allocated on the first page fault
    // by the T_PGFLT handler in trap.c.
    myproc()->sz += n;
  } else if(n < 0){
    // Shrink: deallocate immediately (nothing to skip here because
    // deallocuvm() in vm.c is already guarded against un-mapped PTEs).
    if(growproc(n) < 0)
      return -1;
  }
  // n == 0: no-op, just return current break

  return addr;
}
// ─────────────────────────────────────────────────────────────────────────────

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// ─── ASSIGNMENT 9, QUESTION 1: sys_vmprint ───────────────────────────────
// Thin wrapper: the real work is done by vmprint() declared in defs.h
// and implemented in vm.c.
int
sys_vmprint(void)
{
  return vmprint();
}
// ─────────────────────────────────────────────────────────────────────────────

// ─── ASSIGNMENT 9, QUESTION 2: sys_getfaults ─────────────────────────────
// Return the number of page faults that the kernel has handled on behalf
// of the calling process (stored in proc->page_faults, incremented in
// trap.c's T_PGFLT handler).
int
sys_getfaults(void)
{
  return myproc()->page_faults;
}
// ─────────────────────────────────────────────────────────────────────────────

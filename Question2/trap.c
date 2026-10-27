// trap.c — xv6 trap/interrupt handler
// CHANGES FOR ASSIGNMENT 9, QUESTION 2:
//   Handle T_PGFLT (page fault, trap 14):
//     1. Read the faulting address with rcr2().
//     2. If the address is below myproc()->sz (within the lazy heap):
//          a. Allocate a zeroed page with kalloc() + memset().
//          b. Map it with mappages() using PTE_W | PTE_U.
//          c. Increment proc->page_faults.
//          d. Return — the user process resumes transparently.
//     3. Otherwise: print pid + address and kill the process.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  // ─── ASSIGNMENT 9, QUESTION 2: Page-fault handler ──────────────────
  case T_PGFLT:
  {
    // rcr2() returns the CR2 register which holds the faulting virtual address
    uint fault_va = rcr2();
    struct proc *p = myproc();

    // Align fault address down to page boundary
    uint page_va = PGROUNDDOWN(fault_va);

    if(p != 0 && fault_va < p->sz){
      // Faulting address is within the process's (lazily extended) heap.
      // Allocate and zero a physical page.
      char *mem = kalloc();
      if(mem == 0){
        // Out of memory — kill process
        cprintf("pid %d: out of memory on page fault at 0x%x\n",
                p->pid, fault_va);
        p->killed = 1;
        break;
      }
      memset(mem, 0, PGSIZE);

      // Map the new page into the process's page table
      if(mappages(p->pgdir, (void*)page_va, PGSIZE,
                  V2P(mem), PTE_W | PTE_U) < 0){
        // mappages failed (e.g. page already mapped) — free and kill
        cprintf("pid %d: mappages failed at 0x%x\n", p->pid, fault_va);
        kfree(mem);
        p->killed = 1;
        break;
      }

      // Count this handled fault
      p->page_faults++;

      // Resume the user process — no further action needed
      break;
    } else {
      // Faulting address is outside the process's address space.
      // Print diagnostic and kill the process.
      if(p != 0)
        cprintf("pid %d: page fault at 0x%x (outside sz=0x%x) — killing\n",
                p->pid, fault_va, p->sz);
      else
        cprintf("kernel page fault at 0x%x\n", fault_va);

      if(p != 0)
        p->killed = 1;
      break;
    }
  }
  // ───────────────────────────────────────────────────────────────────────

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}

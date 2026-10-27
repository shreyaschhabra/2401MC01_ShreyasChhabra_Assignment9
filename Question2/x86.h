// x86.h — xv6 x86-specific functions
// This file stubs out inline assembly functions used by the kernel.
// In the actual xv6 build these use real inline asm; here they are
// declared as extern so the file compiles without errors in a
// reference/review context.

// Trap frame: layout pushed by hardware + trapasm.S on a trap
struct trapframe {
  // registers as pushed by pusha
  uint edi;
  uint esi;
  uint ebp;
  uint oesp;      // useless & ignored
  uint ebx;
  uint edx;
  uint ecx;
  uint eax;

  // rest of trap frame
  ushort gs;
  ushort padding1;
  ushort fs;
  ushort padding2;
  ushort es;
  ushort padding3;
  ushort ds;
  ushort padding4;
  uint trapno;

  // below here defined by x86 hardware
  uint err;
  uint eip;
  ushort cs;
  ushort padding5;
  uint eflags;

  // below here only when crossing rings, such as from user to kernel
  uint esp;
  ushort ss;
  ushort padding6;
};

#ifndef X86_H
#define X86_H

#include "types.h"

static inline uint
rcr2(void)
{
  uint val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
  return val;
}

static inline void
lcr3(uint val)
{
  asm volatile("movl %0,%%cr3" : : "r" (val));
}

static inline void
lgdt(void *p, int size)
{
  volatile ushort pd[3];
  pd[0] = size-1;
  pd[1] = (uint)p;
  pd[2] = (uint)p >> 16;
  asm volatile("lgdt (%0)" : : "r" (pd));
}

static inline void
ltr(ushort sel)
{
  asm volatile("ltr %0" : : "r" (sel));
}

static inline void
pushcli(void) { /* stub for reference build */ }
static inline void
popcli(void) { /* stub for reference build */ }

#endif // X86_H

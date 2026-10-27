// pttest.c — Question 1 user test program
//
// Calls vmprint() at start-up and again after calling sbrk(4 * 4096).
// Shows that exactly four new pages appear after the sbrk.
//
// Compile alongside xv6 user programs; add "pttest" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "=== pttest: page table BEFORE sbrk ===\n");
  vmprint();

  // Grow the heap by exactly 4 pages
  char *p = sbrk(4 * 4096);
  if(p == (char*)-1){
    printf(2, "pttest: sbrk failed\n");
    exit();
  }

  // Touch each new page so it actually gets mapped
  // (required if lazy allocation is active — Question 2)
  int i;
  for(i = 0; i < 4; i++){
    p[i * 4096] = 'x';   // one write per page forces mapping
  }

  printf(1, "\n=== pttest: page table AFTER sbrk(4*4096) ===\n");
  vmprint();

  // The second vmprint should show exactly 4 more user pages than
  // the first, confirming that sbrk correctly extended the address space.

  exit();
}

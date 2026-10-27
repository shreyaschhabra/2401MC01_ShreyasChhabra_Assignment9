// faulttest.c — Question 2 user test program
//
// Tests lazy allocation and page-fault counting.
//
// Behaviour:
//   1. Allocates 32 pages with sbrk(32 * 4096) — NO physical pages allocated yet.
//   2. Writes to exactly 5 distinct pages — triggers 5 page faults.
//   3. Calls getfaults() and prints the result; must equal 5.
//   4. Attempts to access an address ABOVE the allocated region
//      to verify the process is killed by the kernel.
//
// Add "faulttest" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int i;
  // Allocate 32 pages lazily (sbrk only advances proc->sz, no physical alloc)
  char *base = sbrk(32 * 4096);
  if(base == (char*)-1){
    printf(2, "faulttest: sbrk failed\n");
    exit();
  }

  printf(1, "faulttest: allocated 32 pages lazily at 0x%x\n", (uint)base);
  printf(1, "faulttest: now touching exactly 5 pages...\n");

  // Write to 5 different pages (indices 0, 5, 10, 20, 31)
  // Each write should trigger exactly one page fault in the kernel.
  int page_indices[5] = {0, 5, 10, 20, 31};
  for(i = 0; i < 5; i++){
    base[page_indices[i] * 4096] = 'A' + i;
  }

  // Read back the fault count via the getfaults() system call
  int faults = getfaults();
  printf(1, "faulttest: page faults handled = %d (expected 5)\n", faults);

  if(faults == 5)
    printf(1, "faulttest: PASS — correct number of page faults\n");
  else
    printf(1, "faulttest: FAIL — expected 5, got %d\n", faults);

  // ── Out-of-bounds access test ──────────────────────────────────────────
  // Access an address 1 page ABOVE the allocated region.
  // The kernel should print an error and kill this process.
  printf(1, "faulttest: now accessing INVALID address (above sz)...\n");

  // NOTE: This write should kill the process. The shell continues
  // because only this process is killed.
  char *bad = base + 32 * 4096 + 4096;  // one page beyond allocated region
  *bad = 'X';   // triggers page fault with address >= proc->sz => kill

  // Should never reach here
  printf(1, "faulttest: ERROR — should have been killed\n");
  exit();
}

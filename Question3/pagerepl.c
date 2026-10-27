// pagerepl.c — Question 3 user program
//
// Simulates FIFO and LRU page replacement on a given reference string.
//
// Usage (inside xv6):
//   pagerepl <frames> <page1> <page2> ...
//
// Example:
//   pagerepl 3 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
//
// The program:
//   a) For each algorithm (FIFO, LRU):
//        - For every reference, prints the current frames and HIT/FAULT.
//        - Prints total page faults at the end.
//   b) Runs both algorithms for 3, 4, and 5 frames on the same reference
//      string and prints a summary table.
//
// Expected results for the reference string above with 3 frames:
//   FIFO: 15 faults   LRU: 12 faults

#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_FRAMES  10
#define MAX_REFS    50

// ─── Helper: print current frame contents ───────────────────────────────────
static void
print_frames(int *frames, int n, int capacity)
{
  int i;
  printf(1, "  [");
  for(i = 0; i < capacity; i++){
    if(i < n)
      printf(1, " %2d", frames[i]);
    else
      printf(1, "  -");
    if(i < capacity - 1) printf(1, " |");
  }
  printf(1, " ]");
}

// ─── FIFO Page Replacement ────────────────────────────────────────────────
// frames[]  : circular queue of frame contents (-1 = empty)
// Returns the number of page faults.
static int
fifo(int *refs, int nrefs, int capacity, int verbose)
{
  int frames[MAX_FRAMES];
  int head = 0;    // next slot to evict (FIFO pointer)
  int count = 0;   // frames in use
  int faults = 0;
  int i, j;

  for(i = 0; i < capacity; i++) frames[i] = -1;

  if(verbose)
    printf(1, "\n--- FIFO (frames=%d) ---\n", capacity);

  for(i = 0; i < nrefs; i++){
    int page = refs[i];
    int hit  = 0;

    // Check if page is already in a frame
    for(j = 0; j < count; j++){
      if(frames[j] == page){ hit = 1; break; }
    }

    if(verbose){
      printf(1, "Ref %2d: ", page);
    }

    if(hit){
      if(verbose){ print_frames(frames, count, capacity); printf(1, "  HIT\n"); }
    } else {
      // Page fault: load page into the FIFO head slot
      frames[head] = page;
      head = (head + 1) % capacity;
      if(count < capacity) count++;
      faults++;
      if(verbose){ print_frames(frames, count, capacity); printf(1, "  FAULT\n"); }
    }
  }

  if(verbose)
    printf(1, "FIFO total page faults: %d\n", faults);

  return faults;
}

// ─── LRU Page Replacement ─────────────────────────────────────────────────
// Uses a timestamp-based approach: each frame stores the page number and
// a "last used" time. On a fault, evict the frame with the smallest time.
// Returns the number of page faults.
static int
lru(int *refs, int nrefs, int capacity, int verbose)
{
  int frames[MAX_FRAMES];
  int lru_time[MAX_FRAMES];   // last access time for each frame slot
  int count  = 0;
  int faults = 0;
  int time   = 0;
  int i, j;

  for(i = 0; i < capacity; i++){ frames[i] = -1; lru_time[i] = 0; }

  if(verbose)
    printf(1, "\n--- LRU (frames=%d) ---\n", capacity);

  for(i = 0; i < nrefs; i++, time++){
    int page = refs[i];
    int hit  = 0;
    int slot = -1;

    // Search for the page in frames
    for(j = 0; j < count; j++){
      if(frames[j] == page){
        hit  = 1;
        slot = j;
        break;
      }
    }

    if(verbose)
      printf(1, "Ref %2d: ", page);

    if(hit){
      // Update last-used time for hit page
      lru_time[slot] = time;
      if(verbose){ print_frames(frames, count, capacity); printf(1, "  HIT\n"); }
    } else {
      // Page fault
      faults++;
      if(count < capacity){
        // Still have free frames
        slot = count++;
      } else {
        // Find least-recently-used frame
        int lru_val = lru_time[0];
        slot = 0;
        for(j = 1; j < capacity; j++){
          if(lru_time[j] < lru_val){
            lru_val = lru_time[j];
            slot    = j;
          }
        }
      }
      frames[slot]   = page;
      lru_time[slot] = time;
      if(verbose){ print_frames(frames, count, capacity); printf(1, "  FAULT\n"); }
    }
  }

  if(verbose)
    printf(1, "LRU  total page faults: %d\n", faults);

  return faults;
}

// ─── main ─────────────────────────────────────────────────────────────────
int
main(int argc, char *argv[])
{
  if(argc < 3){
    printf(2, "Usage: pagerepl <frames> <page1> <page2> ...\n");
    exit();
  }

  int capacity = atoi(argv[1]);
  if(capacity < 1 || capacity > MAX_FRAMES){
    printf(2, "pagerepl: frames must be between 1 and %d\n", MAX_FRAMES);
    exit();
  }

  // Parse the reference string from command-line arguments
  int refs[MAX_REFS];
  int nrefs = 0;
  int i;
  for(i = 2; i < argc && nrefs < MAX_REFS; i++)
    refs[nrefs++] = atoi(argv[i]);

  printf(1, "Reference string (%d refs, %d frames):\n  ", nrefs, capacity);
  for(i = 0; i < nrefs; i++)
    printf(1, "%d ", refs[i]);
  printf(1, "\n");

  // ── Part (a): Detailed trace for each algorithm ───────────────────────
  int fifo_faults = fifo(refs, nrefs, capacity, 1);
  int lru_faults  = lru(refs,  nrefs, capacity, 1);

  printf(1, "\n=== Summary for %d frames ===\n", capacity);
  printf(1, "  FIFO: %d faults  (hit ratio = %d/%d)\n",
         fifo_faults, nrefs - fifo_faults, nrefs);
  printf(1, "  LRU : %d faults  (hit ratio = %d/%d)\n",
         lru_faults,  nrefs - lru_faults,  nrefs);

  // ── Part (c): Comparison table for 3, 4, 5 frames ─────────────────────
  printf(1, "\n=== Comparison Table (FIFO vs LRU) ===\n");
  printf(1, "Frames | FIFO Faults | FIFO Hit Ratio | LRU Faults | LRU Hit Ratio\n");
  printf(1, "-------|-------------|----------------|------------|---------------\n");

  int f;
  for(f = 3; f <= 5; f++){
    int ff = fifo(refs, nrefs, f, 0);
    int lf = lru(refs,  nrefs, f, 0);

    // Express hit ratio as percentage (integer math: *100/nrefs)
    int fhit_pct = (nrefs - ff) * 100 / nrefs;
    int lhit_pct = (nrefs - lf) * 100 / nrefs;

    printf(1, "   %d   |     %2d      |      %2d%%       |     %2d     |     %2d%%\n",
           f, ff, fhit_pct, lf, lhit_pct);
  }

  exit();
}

// pagerepl_standalone.c — standalone host simulation of pagerepl.c
// This compiles and runs on macOS/Linux using standard stdio.
// Used to generate terminal screenshots for the assignment report.
//
// Compile: cc -o pagerepl pagerepl_standalone.c
// Run:     ./pagerepl 3 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES  10
#define MAX_REFS    50

static void
print_frames(int *frames, int n, int capacity)
{
  int i;
  printf("  [");
  for(i = 0; i < capacity; i++){
    if(i < n)
      printf(" %2d", frames[i]);
    else
      printf("  -");
    if(i < capacity - 1) printf(" |");
  }
  printf(" ]");
}

static int
fifo(int *refs, int nrefs, int capacity, int verbose)
{
  int frames[MAX_FRAMES];
  int head = 0, count = 0, faults = 0, i, j;
  for(i = 0; i < capacity; i++) frames[i] = -1;

  if(verbose)
    printf("\n--- FIFO (frames=%d) ---\n", capacity);

  for(i = 0; i < nrefs; i++){
    int page = refs[i], hit = 0;
    for(j = 0; j < count; j++)
      if(frames[j] == page){ hit = 1; break; }

    if(verbose) printf("Ref %2d: ", page);

    if(hit){
      if(verbose){ print_frames(frames, count, capacity); printf("  HIT\n"); }
    } else {
      frames[head] = page;
      head = (head + 1) % capacity;
      if(count < capacity) count++;
      faults++;
      if(verbose){ print_frames(frames, count, capacity); printf("  FAULT\n"); }
    }
  }
  if(verbose) printf("FIFO total page faults: %d\n", faults);
  return faults;
}

static int
lru(int *refs, int nrefs, int capacity, int verbose)
{
  int frames[MAX_FRAMES], lru_time[MAX_FRAMES];
  int count = 0, faults = 0, time = 0, i, j;
  for(i = 0; i < capacity; i++){ frames[i] = -1; lru_time[i] = 0; }

  if(verbose)
    printf("\n--- LRU (frames=%d) ---\n", capacity);

  for(i = 0; i < nrefs; i++, time++){
    int page = refs[i], hit = 0, slot = -1;
    for(j = 0; j < count; j++)
      if(frames[j] == page){ hit = 1; slot = j; break; }

    if(verbose) printf("Ref %2d: ", page);

    if(hit){
      lru_time[slot] = time;
      if(verbose){ print_frames(frames, count, capacity); printf("  HIT\n"); }
    } else {
      faults++;
      if(count < capacity){
        slot = count++;
      } else {
        int lv = lru_time[0]; slot = 0;
        for(j = 1; j < capacity; j++)
          if(lru_time[j] < lv){ lv = lru_time[j]; slot = j; }
      }
      frames[slot] = page;
      lru_time[slot] = time;
      if(verbose){ print_frames(frames, count, capacity); printf("  FAULT\n"); }
    }
  }
  if(verbose) printf("LRU  total page faults: %d\n", faults);
  return faults;
}

int main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(stderr, "Usage: pagerepl <frames> <page1> <page2> ...\n");
    return 1;
  }
  int capacity = atoi(argv[1]);
  int refs[MAX_REFS], nrefs = 0, i;
  for(i = 2; i < argc && nrefs < MAX_REFS; i++)
    refs[nrefs++] = atoi(argv[i]);

  printf("Reference string (%d refs, %d frames):\n  ", nrefs, capacity);
  for(i = 0; i < nrefs; i++) printf("%d ", refs[i]);
  printf("\n");

  int ff = fifo(refs, nrefs, capacity, 1);
  int lf = lru(refs,  nrefs, capacity, 1);

  printf("\n=== Summary for %d frames ===\n", capacity);
  printf("  FIFO: %d faults  (hit ratio = %d/%d)\n", ff, nrefs-ff, nrefs);
  printf("  LRU : %d faults  (hit ratio = %d/%d)\n", lf, nrefs-lf, nrefs);

  printf("\n=== Comparison Table (FIFO vs LRU) ===\n");
  printf("Frames | FIFO Faults | FIFO Hit Ratio | LRU Faults | LRU Hit Ratio\n");
  printf("-------|-------------|----------------|------------|---------------\n");
  int f;
  for(f = 3; f <= 5; f++){
    int ffi = fifo(refs, nrefs, f, 0);
    int lfi = lru(refs,  nrefs, f, 0);
    int fhit = (nrefs - ffi) * 100 / nrefs;
    int lhit = (nrefs - lfi) * 100 / nrefs;
    printf("   %d   |     %2d      |      %2d%%       |     %2d     |     %2d%%\n",
           f, ffi, fhit, lfi, lhit);
  }
  return 0;
}

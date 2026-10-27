# Question 3 — Page Replacement Algorithms (FIFO & LRU)

## How to Run (Two Options)

### Option A: Run on Host Machine (macOS/Linux) — For Testing & Screenshots

The `pagerepl_standalone.c` file compiles and runs on any standard C compiler:

```bash
# Compile
cc -o pagerepl pagerepl_standalone.c

# Run with the assignment reference string
./pagerepl 3 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
```

**Expected output:**
- FIFO total page faults: **15**
- LRU total page faults: **12**

### Option B: Run Inside xv6

`pagerepl.c` uses only xv6 user-space APIs (`types.h`, `user.h`).  
Add it to xv6's `Makefile` under `UPROGS`:

```makefile
UPROGS=\
    ...
    _pagerepl\
```

Then inside xv6:
```
$ pagerepl 3 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
```

## Algorithms

### FIFO (First-In First-Out)
- Maintains a circular queue of frames
- On a page fault: evicts the **oldest loaded page**
- Simple but can suffer from Bélády's anomaly

### LRU (Least Recently Used)
- Each frame stores a timestamp of last access
- On a page fault: evicts the frame with the **smallest timestamp**
- Better performance for workloads with temporal locality

## Results

| Frames | FIFO Faults | FIFO Hit% | LRU Faults | LRU Hit% |
|--------|-------------|-----------|------------|----------|
| 3      | **15**      | 25%       | **12**     | 40%      |
| 4      | **10**      | 50%       | **8**      | 60%      |
| 5      | **9**       | 55%       | **7**      | 65%      |

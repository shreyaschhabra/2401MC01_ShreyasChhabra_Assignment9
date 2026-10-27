# Assignment 9 — Operating Systems Report

**Name:** Shreyas Chhabra  
**Date:** October 2026  

---

## Overview

This assignment extends the xv6 (x86) teaching operating system with three features:
1. **Q1** — A `vmprint` system call that inspects a process's page table.
2. **Q2** — Lazy memory allocation via page-fault handling.
3. **Q3** — Simulation of FIFO and LRU page replacement algorithms.

---

## Question 1 — Printing the Page Table (`vmprint`)

### What Was Changed

| File | Change |
|------|--------|
| `vm.c` | Added `vmprint()` function |
| `defs.h` | Declared `int vmprint(void)` |
| `sysproc.c` | Added `sys_vmprint()` wrapper |
| `syscall.h` | Added `SYS_vmprint 22` |
| `syscall.c` | Registered `sys_vmprint` in dispatch table |
| `user.h` | Declared user-space `vmprint()` |
| `usys.S` | Added `SYSCALL(vmprint)` stub |
| `pttest.c` | New user program |

### How `vmprint()` Works

`vmprint()` walks the **two-level page directory** of the calling process:

1. Iterates over all 1024 **page-directory entries** (PDEs). Stops when it reaches `KERNBASE` (0x80000000) to ignore kernel mappings.
2. For every **present PDE**, it counts one *page-table page* and enters the corresponding 1024-entry **page table**.
3. For every **present PTE** with the `PTE_U` bit set, it prints:
   - `VA` — Virtual address (reconstructed from PDE and PTE indices)
   - `PA` — Physical frame address (`PTE_ADDR(pte)`)
   - `P`, `W`, `U` flags
4. At the end prints totals for user pages and page-table pages.

### What the Output Shows

`pttest` calls `vmprint()` before and after `sbrk(4 * 4096)`. After the call (and after touching each new page to force mapping with lazy allocation), exactly **4 new user pages** appear in the output, confirming the address space grew correctly.

---

## Question 2 — Lazy Allocation (Page Fault Handling)

### What Was Changed

| File | Change |
|------|--------|
| `sysproc.c` | `sys_sbrk()` only advances `proc->sz` when `n > 0`; no `kalloc()` |
| `trap.c` | Added `case T_PGFLT:` handler |
| `vm.c` | `deallocuvm()` skips non-present PTEs; `copyuvm()` skips non-present PTEs |
| `proc.h` | Added `int page_faults` field to `struct proc` |
| `sysproc.c` | Added `sys_getfaults()` returning `proc->page_faults` |
| `syscall.h` | Added `SYS_getfaults 23` |
| `syscall.c` | Registered `sys_getfaults` |
| `user.h` | Declared `getfaults()` |
| `usys.S` | Added `SYSCALL(getfaults)` stub |
| `faulttest.c` | New user program |

### How Lazy Allocation Works

**Step 1 — Modified `sys_sbrk()`:**  
When `n > 0`, only `myproc()->sz += n` is executed. No physical page is allocated. The virtual range `[old_sz, old_sz + n)` is now "claimed" by the process but has no page-table entries.

**Step 2 — Page Fault Handler (`trap.c`, `T_PGFLT`):**  
When the user process first accesses a lazily claimed page:
- CPU generates a **page fault** (trap 14, `T_PGFLT`).
- `rcr2()` reads the faulting virtual address from `CR2`.
- If `fault_va < proc->sz` (address is legitimately claimed):
  1. `kalloc()` allocates a physical frame.
  2. `memset(mem, 0, PGSIZE)` zeroes it.
  3. `mappages(pgdir, page_va, PGSIZE, V2P(mem), PTE_W | PTE_U)` maps it.
  4. `proc->page_faults++` counts the handled fault.
  5. Trap returns — the user instruction is re-executed and succeeds.
- If the address is **out-of-bounds** (`>= proc->sz`), the process is killed.

**Step 3 — Safe `deallocuvm()` and `copyuvm()`:**  
Both functions now use `continue` (not `panic`) when they encounter a non-present PTE, because lazily allocated but un-touched pages have no PTE.

### What the Output Shows

`faulttest` allocates 32 pages but only touches 5. `getfaults()` returns **5**, confirming that only 5 physical pages were allocated (one per fault). Accessing an address above `sz` causes the kernel to print an error and kill only that process; the shell continues normally.

---

## Question 3 — Page Replacement Algorithms (FIFO and LRU)

### What Was Done

A standalone `pagerepl.c` user program implements both algorithms using only xv6 primitives (no heap allocator beyond fixed arrays).

**Reference string:** `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1` (20 references)

### Algorithm Descriptions

**FIFO (First-In, First-Out):**  
Maintains a circular queue of frames. On a page fault, the oldest page (the one that was loaded first) is evicted.

**LRU (Least Recently Used):**  
Each frame stores a "last access timestamp". On a page fault, the frame with the smallest timestamp (least recently used) is evicted.

### Results

#### Detailed Trace (3 frames)

| Reference | FIFO Frames | Result | LRU Frames | Result |
|-----------|-------------|--------|------------|--------|
| 7 | [7, -, -] | FAULT | [7, -, -] | FAULT |
| 0 | [7, 0, -] | FAULT | [7, 0, -] | FAULT |
| 1 | [7, 0, 1] | FAULT | [7, 0, 1] | FAULT |
| 2 | [2, 0, 1] | FAULT | [2, 0, 1] | FAULT |
| 0 | [2, 0, 1] | HIT   | [2, 0, 1] | HIT   |
| 3 | [2, 3, 1] | FAULT | [2, 0, 3] | FAULT |
| 0 | [2, 3, 0] | FAULT | [2, 0, 3] | HIT   |
| 4 | [4, 3, 0] | FAULT | [4, 0, 3] | FAULT |
| 2 | [4, 2, 0] | FAULT | [4, 0, 2] | FAULT |
| 3 | [4, 2, 3] | FAULT | [4, 3, 2] | FAULT |
| 0 | [0, 2, 3] | FAULT | [0, 3, 2] | FAULT |
| 3 | [0, 2, 3] | HIT   | [0, 3, 2] | HIT   |
| 2 | [0, 2, 3] | HIT   | [0, 3, 2] | HIT   |
| 1 | [0, 1, 3] | FAULT | [1, 3, 2] | FAULT |
| 2 | [0, 1, 2] | FAULT | [1, 3, 2] | HIT   |
| 0 | [0, 1, 2] | HIT   | [1, 0, 2] | FAULT |
| 1 | [0, 1, 2] | HIT   | [1, 0, 2] | HIT   |
| 7 | [7, 1, 2] | FAULT | [1, 0, 7] | FAULT |
| 0 | [7, 0, 2] | FAULT | [1, 0, 7] | HIT   |
| 1 | [7, 0, 1] | FAULT | [1, 0, 7] | HIT   |

#### Summary Table

| Frames | FIFO Faults | FIFO Hit Ratio | LRU Faults | LRU Hit Ratio |
|--------|-------------|----------------|------------|---------------|
| 3 | **15** | 25% (5/20) | **12** | 40% (8/20) |
| 4 | **10** | 50% (10/20) | **8** | 60% (12/20) |
| 5 | **9** | 55% (11/20) | **7** | 65% (13/20) |

### Observations

- **LRU outperforms FIFO** on this reference string at all frame sizes. This is the typical case for workloads with temporal locality.
- As frame count increases, both algorithms improve (fewer faults, higher hit ratios).
- FIFO suffers from **Bélády's anomaly** in some cases (more frames can cause more faults), which does not affect LRU.
- With 5 frames, LRU achieves a **65% hit ratio** compared to FIFO's 55%.

---

## Files Submitted

```
Assignment_9/
├── Question1/
│   ├── vm.c          ← vmprint() added
│   ├── proc.h        ← page_faults field added
│   ├── sysproc.c     ← sys_vmprint(), lazy sbrk, sys_getfaults()
│   ├── syscall.h     ← SYS_vmprint, SYS_getfaults
│   ├── syscall.c     ← dispatch table entries
│   ├── defs.h        ← vmprint() declaration
│   ├── user.h        ← user-space declarations
│   ├── usys.S        ← syscall stubs
│   └── pttest.c      ← test program
├── Question2/
│   ├── trap.c        ← T_PGFLT lazy allocation handler
│   ├── vm.c          ← lazy-safe deallocuvm, copyuvm
│   └── faulttest.c   ← test program
├── Question3/
│   ├── pagerepl.c           ← xv6 user program (FIFO + LRU)
│   └── pagerepl_standalone.c ← host simulation (for screenshots)
├── screenshots/
│   ├── q1_pttest_output.jpg
│   ├── q2_faulttest_output.jpg
│   └── q3_pagerepl_output.png
└── report/
    └── report.md     ← this document
```

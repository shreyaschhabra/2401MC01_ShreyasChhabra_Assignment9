# Question 2 — Lazy Allocation & Page Fault Handling

## ⚠️ How to Use These Files

These are **kernel patch files** for [xv6 (x86)](https://github.com/mit-pdos/xv6-public).

> **Note:** Question 2 builds on Question 1. Apply Q1 files first, then apply Q2 files.

### Steps to Apply to xv6

1. **Apply Q1 files first** (see `../Question1/README.md`)

2. **Copy Q2 modified files:**
   ```bash
   cp trap.c  xv6/trap.c
   cp vm.c    xv6/vm.c    # replaces Q1 vm.c with lazy-safe version
   ```

3. **Re-build and run:**
   ```bash
   make qemu
   # Inside xv6:
   $ faulttest
   ```

## What Each File Changes

| File | Change |
|------|--------|
| `sysproc.c` | `sys_sbrk(n>0)` now only advances `proc->sz`, no `kalloc()` |
| `trap.c` | `case T_PGFLT:` handler: reads `rcr2()`, allocates page if `va < proc->sz`, else kills |
| `vm.c` | `deallocuvm()` and `copyuvm()` skip non-present PTEs (no panic for lazy holes) |
| `proc.h` | `int page_faults` field in `struct proc` |
| `faulttest.c` | New user program: allocates 32 pages lazily, touches 5, asserts `getfaults()==5` |

## Key Design: T_PGFLT Handler in trap.c

```c
case T_PGFLT:
{
    uint fault_va = rcr2();          // faulting virtual address
    struct proc *p = myproc();
    uint page_va = PGROUNDDOWN(fault_va);

    if(p != 0 && fault_va < p->sz){ // within lazily claimed range?
        char *mem = kalloc();
        memset(mem, 0, PGSIZE);
        mappages(p->pgdir, (void*)page_va, PGSIZE, V2P(mem), PTE_W|PTE_U);
        p->page_faults++;            // count it
        break;                       // resume process
    } else {
        p->killed = 1;               // out of bounds — kill
        break;
    }
}
```

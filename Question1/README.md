# Question 1 — Printing the Page Table (`vmprint`)

## ⚠️ How to Use These Files

These files are **kernel patch files** for [xv6 (x86)](https://github.com/mit-pdos/xv6-public).  
They are **not standalone programs** and cannot be compiled on their own — they require the full xv6 source tree.

### Steps to Apply to xv6

1. **Clone xv6:**
   ```bash
   git clone https://github.com/mit-pdos/xv6-public.git xv6
   cd xv6
   ```

2. **Copy the modified files** (overwrite the originals in xv6):
   ```bash
   cp vm.c       xv6/vm.c
   cp sysproc.c  xv6/sysproc.c
   cp syscall.h  xv6/syscall.h
   cp syscall.c  xv6/syscall.c
   cp defs.h     xv6/defs.h
   cp proc.h     xv6/proc.h
   cp user.h     xv6/user.h
   cp usys.S     xv6/usys.S
   cp pttest.c   xv6/pttest.c
   ```

3. **Add `pttest` to xv6's `Makefile`** under `UPROGS`:
   ```makefile
   UPROGS=\
       ...
       _pttest\
   ```

4. **Build and run:**
   ```bash
   make qemu
   # Inside xv6:
   $ pttest
   ```

## What Each File Changes

| File | Change |
|------|--------|
| `vm.c` | `vmprint()` added at the bottom — walks PDEs and PTEs, prints VA/PA/flags |
| `defs.h` | `int vmprint(void)` declared in the `// vm.c` section |
| `sysproc.c` | `sys_vmprint()` wrapper + lazy `sys_sbrk()` + `sys_getfaults()` |
| `syscall.h` | `SYS_vmprint 22`, `SYS_getfaults 23` |
| `syscall.c` | `sys_vmprint` and `sys_getfaults` added to dispatch table |
| `user.h` | `int vmprint(void)` and `int getfaults(void)` declared |
| `usys.S` | `SYSCALL(vmprint)` and `SYSCALL(getfaults)` stubs |
| `proc.h` | `int page_faults` field added to `struct proc` |
| `pttest.c` | New user program: calls vmprint before/after sbrk(4*4096) |

## Why the Headers Are Included Here

The xv6 headers (`types.h`, `param.h`, `mmu.h`, `memlayout.h`, `x86.h`, `elf.h`,
`traps.h`, `spinlock.h`, `stat.h`, `date.h`) are included in this folder for
**reference and code review only**, so you can read the kernel files and understand
all macro definitions and type declarations without opening the full xv6 source tree.

They are **not** sufficient to compile `vm.c` standalone because functions like
`kalloc()`, `kfree()`, `myproc()`, `mycpu()`, `cprintf()`, etc. are defined across
multiple other xv6 source files (kalloc.c, proc.c, console.c).

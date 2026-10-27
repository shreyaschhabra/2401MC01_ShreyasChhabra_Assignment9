// vm.c — xv6 virtual memory management
// CHANGES FOR ASSIGNMENT 9, QUESTION 2:
//   • deallocuvm(): Skip pages that were never allocated (no PTE_P) instead
//     of calling panic(). This is required for lazy allocation because sbrk()
//     now only advances proc->sz without mapping physical pages.
//   • copyuvm(): Same guard — skip un-present PTEs instead of panic().
//
// CHANGES FOR ASSIGNMENT 9, QUESTION 1 (also present here for reference):
//   • vmprint(): walks the page directory and prints user-space mappings.

#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

extern char data[];
pde_t *kpgdir;

void
seginit(void)
{
  struct cpu *c;
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    memset(pgtab, 0, PGSIZE);
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W},
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W},
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W},
};

pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

void
switchkvm(void)
{
  lcr3(V2P(kpgdir));
}

void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));
  popcli();
}

void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;
  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, P2V(pa), offset+i, n) != n)
      return -1;
  }
  return 0;
}

int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}

// ─── ASSIGNMENT 9, QUESTION 2: deallocuvm — lazy-allocation safe ─────────
// Original code: panic("kfree") if pa == 0, and also panics inside the loop
// if a page is "not present" because it was supposed to be mapped.
// NEW behaviour: if the PTE is not present (page was never lazily allocated),
// simply skip it — do NOT call kfree.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;  // skip rest of this PDE
    else if((*pte & PTE_P) != 0){
      // Page is present — free the physical frame
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
    // else: PTE exists but not present — was never lazily allocated, skip.
  }
  return newsz;
}
// ─────────────────────────────────────────────────────────────────────────────

void
freevm(pde_t *pgdir)
{
  uint i;
  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char*)pgdir);
}

void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;
  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// ─── ASSIGNMENT 9, QUESTION 2: copyuvm — lazy-allocation safe ────────────
// Original code: panic("copyuvm: page not present") if PTE_P is not set.
// NEW behaviour: if the PTE is not present, skip it (the child will also
// fault and allocate lazily).
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      continue;   // No page table for this range — skip (lazy hole)
    if(!(*pte & PTE_P))
      continue;   // PTE exists but not present — skip (lazily unallocated)
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}
// ─────────────────────────────────────────────────────────────────────────────

char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;
  pte = walkpgdir(pgdir, uva, 0);
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va += n;
    va0 += PGSIZE;
  }
  return 0;
}

// ─── ASSIGNMENT 9, QUESTION 1: vmprint() ────────────────────────────────────
int
vmprint(void)
{
  struct proc *p = myproc();
  pde_t *pgdir   = p->pgdir;
  uint   user_pages = 0;
  uint   pt_pages   = 0;
  uint   i, j;

  cprintf("Page table for pid %d (%s):\n", p->pid, p->name);
  cprintf("  %-12s  %-12s  %s\n", "VirtAddr", "PhysAddr", "P W U");

  for(i = 0; i < NPDENTRIES; i++){
    pde_t pde = pgdir[i];
    if(!(pde & PTE_P))
      continue;
    uint pde_va = i << PDXSHIFT;
    if(pde_va >= KERNBASE)
      break;
    pt_pages++;
    pte_t *pgtab = (pte_t*)P2V(PTE_ADDR(pde));
    for(j = 0; j < NPTENTRIES; j++){
      pte_t pte = pgtab[j];
      if(!(pte & PTE_P))
        continue;
      uint va = (i << PDXSHIFT) | (j << PTXSHIFT);
      if(va >= KERNBASE)
        continue;
      uint pa    = PTE_ADDR(pte);
      int  flag_p = (pte & PTE_P) ? 1 : 0;
      int  flag_w = (pte & PTE_W) ? 1 : 0;
      int  flag_u = (pte & PTE_U) ? 1 : 0;
      cprintf("  VA=0x%08x  PA=0x%08x  P=%d W=%d U=%d\n",
              va, pa, flag_p, flag_w, flag_u);
      if(flag_u)
        user_pages++;
    }
  }

  cprintf("Total user pages : %d\n", user_pages);
  cprintf("Page-table pages : %d\n", pt_pages);
  return 0;
}
// ─────────────────────────────────────────────────────────────────────────────

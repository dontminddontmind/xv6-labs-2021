// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"

#include "param.h"

#include "memlayout.h"

#include "spinlock.h"

#include "riscv.h"

#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct ref_stru {
  struct spinlock lock;
  int cnt[PHYSTOP / PGSIZE];
} ref;

void kinit() {
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    ref.cnt[(uint64)p / PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // TODO check?
  acquire(&ref.lock);
  if (--ref.cnt[(uint64)pa / PGSIZE] == 0) {
    release(&ref.lock);

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  } else {
    release(&ref.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    acquire(&ref.lock);
    ref.cnt[(uint64)r / PGSIZE] = 1; // 将引用计数初始化为1
    release(&ref.lock);
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

/**
 * @brief cowpage 判断一个页面是否为COW页面
 * @param pagetable 指定查询的页表
 * @param va 虚拟地址
 * @return 0 是 -1 不是
 */
int cowpage(pagetable_t pagetable, uint64 va) {
  if (va >= MAXVA)
    return -1;
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0)
    return -1;
  if ((*pte & PTE_V) == 0)
    return -1;
  return (*pte & PTE_F ? 0 : -1);
}

// TODO: check?
int krefcnt(void *pa) { return ref.cnt[(uint64)pa / PGSIZE]; }

void *cowalloc(pagetable_t pagetable, uint64 va) {
  if (va % PGSIZE != 0)
    return 0;

  uint64 pa = walkaddr(pagetable, va);
  if (pa == 0)
    return 0;

  pte_t *pte = walk(pagetable, va, 0);

  // TODO: read only?
  if (krefcnt((char *)pa) == 1) {
    *pte |= PTE_W;
    *pte &= ~PTE_F;
    return (void *)pa;
  } else {
    char *newpage = kalloc();
    if (newpage == 0)
      return 0;

    memmove(newpage, (char *)pa, PGSIZE);

    *pte &= ~PTE_V;

    if (mappages(pagetable, va, PGSIZE, (uint64)newpage,
                 (PTE_FLAGS(*pte) | PTE_W) & ~PTE_F) != 0) {
      kfree(newpage);
      *pte |= PTE_V;
      // TODO: why?
      return 0;
    }

    kfree((char *)PGROUNDDOWN(pa)); // why?
    return newpage;
  }
}

int kaddrefcnt(void *pa) {
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&ref.lock);
  ++ref.cnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  return 0;
}

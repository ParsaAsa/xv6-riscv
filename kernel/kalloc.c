// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

int refcnt[PHYSTOP / PGSIZE];
struct spinlock refcnt_lock;

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

void
kinit()
{ 
  initlock(&refcnt_lock, "refcnt");
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    acquire(&refcnt_lock);
    refcnt[(uint64)p / PGSIZE] = 0;  // start with 0, truly free
    release(&refcnt_lock);
    kfree(p);
  }
}


//////////////////////

void incref(uint64 pa) {
  acquire(&refcnt_lock);
  refcnt[pa / PGSIZE]++;
  release(&refcnt_lock);
}
void decref(uint64 pa) {
  acquire(&refcnt_lock);
  if(--refcnt[pa / PGSIZE] == 0){
    release(&refcnt_lock);
    kfree((void*)pa);   // actually free the page
  } else {
    release(&refcnt_lock);
  }
}


/////////////////////


// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}



// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  struct proc *p = myproc();

  // --- Step 1: Standard Allocation Attempt ---
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  // --- Step 2: Out of Memory (OOM) Handling ---
  if(r == 0){
    /* Safety Guards: 
       1. p == 0: We are in early boot (main/kinit). Cannot swap yet.
       2. p->is_kproc: The swap worker itself or other kernel threads shouldn't trigger swap.
       3. p->is_swapping: Prevent recursive loops where kalloc -> swap -> kalloc.
    */
    if(p != 0 && p->is_kproc == 0 && p->is_swapping == 0){
      p->is_swapping = 1; // Set the flag to protect this process

      acquire(&swap_lock);
      // Wake up the swap_out_worker (Phase 2)
      global_swap_req.is_active = 1;
      global_swap_req.p = p; 
      wakeup(&global_swap_req);

      // Wait for the worker to finish evicting a page
      while(global_swap_req.is_active == 1){
        sleep(&global_swap_req, &swap_lock);
      }
      release(&swap_lock);

      p->is_swapping = 0; // Unset the flag

      // --- Step 3: Final Attempt ---
      // Try to grab the page the worker just freed
      acquire(&kmem.lock);
      r = kmem.freelist;
      if(r)
        kmem.freelist = r->next;
      release(&kmem.lock);
    }
  }

  // --- Step 4: Initialization ---
  if(r){
    // Fill with junk (5) to catch dangling references
    memset((char*)r, 5, PGSIZE);

    // Initialize reference count to 1
    acquire(&refcnt_lock);
    refcnt[(uint64)r / PGSIZE] = 1;
    release(&refcnt_lock);
  }

  return (void*)r;
}









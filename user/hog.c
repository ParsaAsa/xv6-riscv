#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("Hog: Starting to consume memory...\n");

  for(int i = 0; i < 1000; i++){
    char *p = sbrk(4096); // Request one page
    if(p == (char*)-1){
      printf("Hog: sbrk failed at page %d. Waiting for swap...\n", i);
      for(volatile int volatile_counter = 0; volatile_counter < 1000000; volatile_counter++);
      continue;
    }
    
    // Crucial: We MUST touch the page to ensure it's actually allocated
    // and to set the PTE_A (Accessed) bit.
    *p = 'z'; 
    
    if(i % 10 == 0)
      printf("Hog: Allocated %d pages\n", i);
  }

  printf("Hog: Done.\n");
  exit(0);
}
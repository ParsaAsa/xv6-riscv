#include "kernel/types.h"
#include "user/user.h"

int main() {
    printf("Testswap: Starting...\n");
    char *p = sbrk(4096 * 100); // Grab 100 pages
    
    // Write something specific to the first page
    p[0] = 'A';
    p[1] = 'B';
    
    printf("Testswap: Filling memory to force page 0 into swap...\n");
    // Write to all other pages to push the first one out
    for(int i = 1; i < 100; i++) {
        p[i * 4096] = (char)i;
    }

    printf("Testswap: Attempting to read back page 0 (should trigger Swap-In)...\n");
    if(p[0] == 'A' && p[1] == 'B') {
        printf("Testswap: SUCCESS! Data recovered from swap.\n");
    } else {
        printf("Testswap: FAILURE! Data corrupted.\n");
    }
    
    exit(0);
}
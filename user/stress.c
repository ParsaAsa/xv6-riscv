#include "kernel/types.h"
#include "user/user.h"

#define FOUR_MB (4 * 1024 * 1024)
#define PGSIZE 4096

int main(int argc, char *argv[]) {
    char *p;
    // 'volatile' tells the compiler: "Don't optimize this variable!"
    volatile long int counter = 0; 

    p = sbrk(FOUR_MB);
    if(p == (char*)-1) {
        printf("sbrk failed\n");
        exit(1);
    }

    for(int i = 0; i < FOUR_MB; i += PGSIZE) {
        p[i] = 'X'; 
    }

    printf("Stress: Spinning for real now...\n");

    for(uint64 j = 0; j < 5000000000ULL; j++) {
        counter++;
        // Every once in a while, print something or touch memory
        // to prove we are still alive and working
        if(j % 10000000 == 0){
            p[j % FOUR_MB] = (char)(j % 255);
        }
    }

    printf("Stress finished. Final counter: %ld\n", counter);
    exit(0);
}
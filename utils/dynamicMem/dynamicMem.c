#include <stdio.h>
#include <stdlib.h>



void* creDy(size_t n, size_t size) {
    void* mem = calloc(n, size);
    if(mem == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    } 

    return mem;
}


void* reDy(void* src, size_t size) {
    void* newMem = realloc(src, size);
    if(newMem == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    return newMem;
}
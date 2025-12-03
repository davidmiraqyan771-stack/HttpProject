#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>

void* creDy(size_t n, size_t size);
void* reDy(void* src, size_t size);
char *reciveTheReqOrRes(size_t startSize, int clientSock);



#endif
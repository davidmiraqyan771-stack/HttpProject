#ifndef SUTILS_H
#define SUTILS_H
#include <stdio.h>

struct Request
{
    char *type;
    char *path;
    char *body;
    char *protocol;
    char *agent;
    size_t size;
    size_t time;
};

char *formingResponse(struct Request *req, int statusCode, char *statusCodeMessage);
FILE *openFile(char *pathname, char *mode);
void *GETfunc(struct Request* req, int clientSock);



#endif
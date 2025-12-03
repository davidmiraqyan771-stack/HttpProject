#include "serverUtils.h"
#include <string.h>
#include <time.h>
#include "../../utils/utils.h"

char *formingResponse(struct Request* req1, int statusCode, char *statusCodeMessage)
{
    char *response = creDy((strlen(req1->body) + strlen(req1->path) + strlen(req1->type) + strlen(req1->protocol) + strlen(statusCodeMessage)) * 2, sizeof(char));
    sprintf(response, "%s %d %s\r\n", req1->protocol, statusCode, statusCodeMessage);
    sprintf(response + strlen(response), "Body-size: %ld\r\n", req1->size);
    sprintf(response + strlen(response), "Time: %lds\r\n\r\n", time(NULL) - req1->time);
    sprintf(response + strlen(response), "%s", req1->body);

    return response;
}

FILE *openFile(char *pathname, char *mode)
{
    if (strstr(pathname, "../") != NULL)
    {
        return NULL;
    }
    char *path = creDy(strlen(pathname) + strlen("data"), sizeof(char));
    strcat(path, "data");
    strcat(path, pathname + 1);
    FILE *file = fopen(path, mode);
    return file;
}

void *GETfunc(struct Request* req, int clientSock)
{
    FILE *file = openFile(req->path, "rb");
    if (file == NULL)
    {
        req->size = 0;
        char *fiResponse = formingResponse(&req, 404, "Resource not found");
        send(clientSock, fiResponse, strlen(fiResponse), 0);
        free(req->type);
        free(req->path);
        free(req->protocol);
        return NULL;
    }

    fseek(file, 0, SEEK_END);

    req->size = ftell(file);
    req->body = reDy(req->body, req->size);
    fseek(file, 0, SEEK_SET);
    fread(req->body, sizeof(char), req->size, file);
    fclose(file);
}


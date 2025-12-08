#include "serverUtils.h"
#include <string.h>
#include "../../utils/utils.h"
#include <time.h>

char *formingResponse(struct Request *req1, int statusCode, char *statusCodeMessage)
{
    char *response = (char *)creDy((strlen(req1->body) + strlen(req1->path) + strlen(req1->type) + strlen(req1->protocol) + strlen(statusCodeMessage)) * 2 + 300, sizeof(char));
    sprintf(response, "%s %d %s\r\n", req1->protocol, statusCode, statusCodeMessage);
    sprintf(response + strlen(response), "Body-size: %ld\r\n", req1->size);
    sprintf(response + strlen(response), "Time: %lds\r\n\r\n", time(NULL) - req1->time);
    sprintf(response + strlen(response), "%s", req1->body);

    return response;
}

FILE *openFile(char *pathname, char *mode)
{
    if (strstr(pathname, "../") != NULL || pathname[1] != '/')
    {
        return NULL;
    }
    char *path = (char *)creDy(strlen(pathname) + strlen("data"), sizeof(char));
    strcat(path, "data");
    strcat(path, pathname + 1);
    FILE *file = fopen(path, mode);
    return file;
}

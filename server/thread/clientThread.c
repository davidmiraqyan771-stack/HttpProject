#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include "../serverUtils/serverUtils.h"
#include "../../utils/utils.h"

#define SUPPORTED_PROTOCOLS "TTP/1.0"
#define DEFAULT_AGENT "TTP/1.0/CLIENT"
#define START_REQUEST_SIZE 20

extern int clientCount;
extern pthread_mutex_t mutex;

void *clientThread(void *arg)
{
    int clientSock = *((int *)arg);
    free(arg);

    while (1)
    {
        struct Request req;
        fflush(stdout);
        char *buf = reciveTheReqOrRes(START_REQUEST_SIZE, clientSock);
        if (buf == NULL)
        {
            printf("Client disconnected\n");
            close(clientSock);

            pthread_mutex_lock(&mutex);
            clientCount--;
            pthread_mutex_unlock(&mutex);

            break;
        }

        req.type = (char *)creDy(strlen(buf) + 1, sizeof(char));
        req.path = (char *)creDy(strlen(buf) + 1, sizeof(char));
        req.protocol = (char *)creDy(strlen(buf) + 1, sizeof(char));
        req.agent = (char *)creDy(strlen(buf) + 1, sizeof(char));
        req.body = (char *)creDy(1, sizeof(char));
        req.size = 0;
        req.time = 0;
        req.append = 0;
        sscanf(buf, "%s %s %s %*s %s %*s %ld %*s %ld %*s %hhd", req.type, req.path, req.protocol, req.agent, &req.size, &req.time, &req.append);
        req.type = reDy(req.type, strlen(req.type) + 1);
        req.path = reDy(req.path, strlen(req.path) + 1);
        req.protocol = reDy(req.protocol, strlen(req.protocol) + 1);
        req.agent = reDy(req.agent, strlen(req.agent) + 1);
        char done = 0;
        char *fiResponse = NULL;
        printf("Buf:\n%s\n",buf);
        if (strstr(SUPPORTED_PROTOCOLS, req.protocol) == NULL)
        {
            req.size = 0;
            strcpy(req.protocol, "TTP/1.0");
            fiResponse = formingResponse(&req, 405, "This Protocol version is not supported");
            done = 1;
        }
        else
        {

            if (strcmp(req.type, "GET") == 0 || strcmp(req.type + 1, "GET") == 0)
            {
                FILE *file = openFile(req.path, "rb");

                if (!file)
                {
                    req.size = 0;
                    fiResponse = formingResponse(&req, 404, "Resource not found");
                }
                else
                {
                    fseek(file, 0, SEEK_END);
                    req.size = ftell(file);
                    req.body = reDy(req.body, req.size + 1);
                    fseek(file, 0, SEEK_SET);
                    fread(req.body, 1, req.size, file);
                    req.body[req.size] = '\0';
                    fclose(file);

                    fiResponse = formingResponse(&req, 200, "OK");
                }
            }
            else if (strcmp(req.type, "POST") == 0 || strcmp(req.type + 1, "POST") == 0)
            {
                req.body = reDy(req.body, req.size + 1);

                char *bodyAddr = strrchr(buf, '\1');
                if (bodyAddr)
                {
                    bodyAddr++;
                    size_t len = strlen(bodyAddr);
                    if (len > req.size)
                    {
                        len = req.size;
                    }
                    memcpy(req.body, bodyAddr, len);
                    req.body[len] = '\0';
                }
                else
                {
                    req.size = 0;
                    req.body[0] = '\0';
                    printf("Bad 4000");
                    fiResponse = formingResponse(&req, 400, "Bad request");
                }

                if (strcmp(req.body, "EXIT") == 0)
                {
                    done = 1;
                }
                else
                {
                    FILE *file = openFile(req.path, (req.append ? "a" : "w"));

                    if (!file)
                    {
                        req.size = 0;
                        req.body[0] = '\0';

                        fiResponse = formingResponse(
                            &req,
                            (errno != ENONET ? 403 : 404),
                            (errno != ENONET ? "Forbidden" : "Resource not found"));
                    }
                    else
                    {
                        fwrite(req.body, 1, strlen(req.body), file);
                        fclose(file);

                        req.body = reDy(req.body, 2);
                        req.body[0] = '\0';
                        req.size = 0;

                        fiResponse = formingResponse(&req, 200, "OK");
                    }
                }
            }
            else if (strcmp(req.type, "ECHO") == 0 || strcmp(req.type + 1, "ECHO") == 0)
            {
                req.body = reDy(req.body, req.size + 1);

                char *bodyAddr = strrchr(buf, '\1');
                if (!bodyAddr)
                {
                    req.size = 0;
                    req.body[0] = '\0';
                    fiResponse = formingResponse(&req, 400, "Bad request");
                }
                else
                {
                    bodyAddr++;
                    size_t len = strlen(bodyAddr);
                    if (len > req.size)
                    {
                        len = req.size;
                    }

                    memcpy(req.body, bodyAddr, len);
                    req.body[len] = '\0';

                    fiResponse = formingResponse(&req, 200, "OK");
                }
            }
            else
            {
                printf("bad %s\n", req.type);
                req.size = 0;
                fiResponse = formingResponse(&req, 400, "Bad request");
            }
        }

        if (fiResponse)
        {
            send(clientSock, fiResponse, strlen(fiResponse), 0);
        }

        free(buf);
        free(req.type);
        free(req.path);
        free(req.protocol);
        free(req.agent);
        free(req.body);
        free(fiResponse);

        if (done)
        {
            printf("Client disconnected\n");
            close(clientSock);

            pthread_mutex_lock(&mutex);
            clientCount--;
            pthread_mutex_unlock(&mutex);

            break;
        }
    }
}
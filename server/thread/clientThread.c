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

        char *buf = reciveTheReqOrRes(START_REQUEST_SIZE, clientSock);
        if (buf == NULL)
        {
            printf("Client Disconnected\n");
            break;
        }
        buf[strlen(buf)] = '\0';

        req.type = creDy(strlen(buf), sizeof(char));
        req.path = creDy(strlen(buf), sizeof(char));
        req.protocol = creDy(strlen(buf), sizeof(char));
        req.agent = creDy(strlen(buf), sizeof(char));
        req.body = creDy(1, sizeof(char));

        sscanf(buf, "%s %s %s %*s %s %*s %ld %*s %ld", req.type, req.path, req.protocol, req.agent, &req.size, &req.time);
        req.type = reDy(req.type, strlen(req.type) + 1);
        req.path = reDy(req.path, strlen(req.path) + 1);
        req.protocol = reDy(req.protocol, strlen(req.protocol) + 1);
        req.agent = reDy(req.agent, strlen(req.agent) + 1);

        if (strstr(SUPPORTED_PROTOCOLS, req.protocol) != NULL)
        {
            free(req.agent);
            if (strcmp(req.type, "GET") == 0 || strcmp(req.type + 1, "GET") == 0)
            {
              if(GETfunc(&req, clientSock) == NULL) {
                free(buf);
                continue;
              }
            }
            else if (strcmp(req.type, "POST") == 0 || strcmp(req.type + 1, "POST") == 0)
            {
                req.body = reDy(req.body, req.size);
                char *bodyAddr = strrchr(buf, '\1');
                bodyAddr += 1;
                bodyAddr[req.size] = '\0';
                strcat(req.body, bodyAddr);
                if (strcmp(req.body, "EXIT") == 0)
                {
                    printf("Client Disconnected\n");
                    free(buf);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    break;
                }
                FILE *file = openFile(req.path, "w");
                if (file == NULL)
                {
                    req.size = 0;
                    strcpy(req.body, "");
                    if (errno != ENONET)
                    {
                        char *fiResponse = formingResponse(&req, 403, "Forbidden");
                        send(clientSock, fiResponse, strlen(fiResponse), 0);
                    }
                    else
                    {
                        char *fiResponse = formingResponse(&req, 404, "Resource not found");
                        send(clientSock, fiResponse, strlen(fiResponse), 0);
                    }
                    free(buf);
                    free(req.body);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    continue;
                }
                fwrite(req.body, sizeof(char), strlen(req.body), file);
                req.body = reDy(req.body, sizeof(char));
                req.size = 0;
                fclose(file);
            }
            else if (strcmp(req.type, "ECHO") == 0 || strcmp(req.type + 1, "ECHO") == 0)
            {
                req.body = reDy(req.body, req.size);
                char *bodyAddr = strrchr(buf, '\1');

                if (bodyAddr == NULL)
                {
                    req.size = 0;
                    strcpy(req.body, "");
                    char *fiResponse = formingResponse(&req, 400, "Bad request");
                    send(clientSock, fiResponse, strlen(fiResponse), 0);
                    free(buf);
                    free(req.body);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    continue;
                }

                bodyAddr += 1;
                bodyAddr[req.size] = '\0';
                strcat(req.body, bodyAddr);
            }
            else if (strcmp(req.type, "APPEND") == 0 || strcmp(req.type + 1, "APPEND") == 0)
            {
                req.body = reDy(req.body, req.size);
                char *bodyAddr = strrchr(buf, '\1');
                bodyAddr += 1;
                bodyAddr[req.size] = '\0';
                strcat(req.body, bodyAddr);
                if (strcmp(req.body, "EXIT") == 0)
                {
                    printf("Client Disconnected\n");
                    free(buf);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    break;
                }
                FILE *file = openFile(req.path, "a");
                if (file == NULL)
                {
                    req.size = 0;
                    strcpy(req.body, "");
                    if (errno != ENONET)
                    {
                        char *fiResponse = formingResponse(&req, 403, "Forbidden");
                        send(clientSock, fiResponse, strlen(fiResponse), 0);
                    }
                    else
                    {
                        char *fiResponse = formingResponse(&req, 404, "Resource not found");
                        send(clientSock, fiResponse, strlen(fiResponse), 0);
                    }
                    free(buf);
                    free(req.body);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    continue;
                }
                fwrite(req.body, sizeof(char), strlen(req.body), file);
                req.body = reDy(req.body, sizeof(char));
                req.size = 0;
                fclose(file);
            }
            else
            {
                req.size = 0;
                char *fiResponse = formingResponse(&req, 400, "Bad request");
                send(clientSock, fiResponse, strlen(fiResponse), 0);
                free(buf);
                free(req.type);
                free(req.path);
                free(req.protocol);
                continue;
            }

            char *fiResponse = formingResponse(&req, 200, "OK");
            send(clientSock, fiResponse, strlen(fiResponse), 0);
            free(fiResponse);
            free(buf);
            free(req.type);
            free(req.path);
            free(req.protocol);
        }
        else
        {
            free(buf);
            free(req.agent);
            req.size = 0;
            strcpy(req.protocol, "TTP/1.0");
            char *fiResponse = formingResponse(&req, 405, "This Protocol version is not supported");
            send(clientSock, fiResponse, strlen(fiResponse), 0);
            free(req.type);
            free(req.path);
            free(req.protocol);
            break;
        }
    }
    pthread_mutex_lock(&mutex);
    clientCount--;
    pthread_mutex_unlock(&mutex);
    close(clientSock);
}
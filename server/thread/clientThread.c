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
    printf("[THREAD] Started\n");

    int clientSock = *((int *)arg);
    printf("[THREAD] Client socket = %d\n", clientSock);
    free(arg);

    while (1)
    {
        printf("[WAIT] Waiting for request...\n");
        struct Request req;

        char *buf = reciveTheReqOrRes(START_REQUEST_SIZE, clientSock);
        if (buf == NULL)
        {
            printf("[INFO] Client disconnected (reciveTheReqOrRes returned NULL)\n");
            break;
        }

        printf("[RECV] Raw buffer: \"%s\"\n", buf);

        req.type = creDy(strlen(buf) + 1, sizeof(char));
        req.path = creDy(strlen(buf) + 1, sizeof(char));
        req.protocol = creDy(strlen(buf) + 1, sizeof(char));
        req.agent = creDy(strlen(buf) + 1, sizeof(char));
        req.body = creDy(1, sizeof(char));
        req.size = 0;
        req.time = 0;

        printf("[PARSE] Running sscanf...\n");
        int scanned = sscanf(buf, "%s %s %s %*s %s %*s %ld %*s %ld",
                             req.type, req.path, req.protocol,
                             req.agent, &req.size, &req.time);

        printf("[PARSE] sscanf scanned %d fields\n", scanned);
        printf("[PARSE] type=\"%s\", path=\"%s\", proto=\"%s\", agent=\"%s\", size=%ld, time=%ld\n",
               req.type, req.path, req.protocol, req.agent, req.size, req.time);

        req.type = reDy(req.type, strlen(req.type) + 1);
        req.path = reDy(req.path, strlen(req.path) + 1);
        req.protocol = reDy(req.protocol, strlen(req.protocol) + 1);
        req.agent = reDy(req.agent, strlen(req.agent) + 1);

        printf("[REALLOC] After reDy: type=%s path=%s protocol=%s agent=%s\n",
               req.type, req.path, req.protocol, req.agent);

        if (strstr(SUPPORTED_PROTOCOLS, req.protocol) != NULL)
        {
            printf("[PROTO] Supported protocol: %s\n", req.protocol);

            /* GET */
            if (strcmp(req.type, "GET") == 0 || strcmp(req.type + 1, "GET") == 0)
            {
                printf("[GET] Path: %s\n", req.path);

                FILE *file = openFile(req.path, "rb");
                if (file == NULL)
                {
                    printf("[GET] File not found\n");
                    req.size = 0;
                    char *fiResponse = formingResponse(&req, 404, "Resource not found");
                    printf("[SEND] 404 response\n");
                    send(clientSock, fiResponse, strlen(fiResponse), 0);
                    free(fiResponse);
                    goto free_loop;
                }

                printf("[GET] File opened\n");
                printf("[GET] File size = %ld\n", req.size);
                fseek(file, 0, SEEK_END);
                printf("[GET] File END\n");
                req.size = ftell(file);
                printf("[GET] File size = %ld\n", req.size);

                req.body = reDy(req.body, req.size + 1);
                fseek(file, 0, SEEK_SET);
                fread(req.body, sizeof(char), req.size, file);
                req.body[req.size] = '\0';
                fclose(file);

                printf("[GET] Body loaded: %ld bytes\n", req.size);
            }

            /* POST */
            else if (strcmp(req.type, "POST") == 0 || strcmp(req.type + 1, "POST") == 0)
            {
                printf("[POST] Path: %s\n", req.path);

                req.body = reDy(req.body, req.size + 1);

                char *bodyAddr = strrchr(buf, '\1');
                printf("[POST] body marker = %p\n", (void *)bodyAddr);

                if (bodyAddr != NULL)
                {
                    bodyAddr++;
                    size_t actualLen = strlen(bodyAddr);
                    printf("[POST] raw body len = %ld\n", actualLen);
                    if (actualLen > req.size)
                        actualLen = req.size;
                    memcpy(req.body, bodyAddr, actualLen);
                    req.body[actualLen] = '\0';
                }
                else
                {
                    printf("[POST] No body found\n");
                    req.body[0] = '\0';
                }

                printf("[POST] Body = \"%s\"\n", req.body);

                if (strcmp(req.body, "EXIT") == 0)
                {
                    printf("[POST] EXIT received\n");
                    free(buf);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    free(req.agent);
                    free(req.body);
                    break;
                }

                FILE *file = openFile(req.path, "w");
                if (file == NULL)
                {
                    printf("[POST] openFile failed errno=%d\n", errno);
                    req.size = 0;
                    req.body[0] = '\0';

                    char *fiResponse = formingResponse(&req,
                                                       (errno != ENONET ? 403 : 404),
                                                       (errno != ENONET ? "Forbidden" : "Resource not found"));

                    printf("[POST] Sending error response\n");
                    send(clientSock, fiResponse, strlen(fiResponse), 0);
                    free(fiResponse);
                    goto free_loop;
                }

                printf("[POST] Writing body (%ld bytes)\n", strlen(req.body));
                fwrite(req.body, sizeof(char), strlen(req.body), file);
                fclose(file);

                req.body = reDy(req.body, 2);
                req.body[0] = '\0';
                req.size = 0;
            }

            /* ECHO */
            else if (strcmp(req.type, "ECHO") == 0 || strcmp(req.type + 1, "ECHO") == 0)
            {
                printf("[ECHO]\n");

                req.body = reDy(req.body, req.size + 1);

                char *bodyAddr = strrchr(buf, '\1');
                printf("[ECHO] body marker = %p\n", (void *)bodyAddr);

                if (!bodyAddr)
                {
                    printf("[ECHO] No body found -> 400\n");
                    req.size = 0;
                    req.body[0] = '\0';
                    char *fiResponse = formingResponse(&req, 400, "Bad request");
                    send(clientSock, fiResponse, strlen(fiResponse), 0);
                    free(fiResponse);
                    goto free_loop;
                }

                bodyAddr++;
                size_t actualLen = strlen(bodyAddr);
                if (actualLen > req.size)
                    actualLen = req.size;
                memcpy(req.body, bodyAddr, actualLen);
                req.body[actualLen] = '\0';

                printf("[ECHO] Body = \"%s\"\n", req.body);
            }

            /* APPEND */
            else if (strcmp(req.type, "APPEND") == 0 || strcmp(req.type + 1, "APPEND") == 0)
            {
                printf("[APPEND] Path: %s\n", req.path);

                req.body = reDy(req.body, req.size + 1);
                char *bodyAddr = strrchr(buf, '\1');

                if (bodyAddr)
                {
                    bodyAddr++;
                    size_t actualLen = strlen(bodyAddr);
                    if (actualLen > req.size)
                        actualLen = req.size;
                    memcpy(req.body, bodyAddr, actualLen);
                    req.body[actualLen] = '\0';
                }
                else
                {
                    req.body[0] = '\0';
                }

                printf("[APPEND] Body=\"%s\"\n", req.body);

                if (strcmp(req.body, "EXIT") == 0)
                {
                    printf("[APPEND] EXIT received\n");
                    free(buf);
                    free(req.type);
                    free(req.path);
                    free(req.protocol);
                    free(req.body);
                    free(req.agent);
                    break;
                }

                FILE *file = openFile(req.path, "a");
                if (file == NULL)
                {
                    printf("[APPEND] openFile failed errno=%d\n", errno);
                    req.size = 0;
                    req.body[0] = '\0';

                    char *fiResponse = formingResponse(&req,
                                                       (errno != ENONET ? 403 : 404),
                                                       (errno != ENONET ? "Forbidden" : "Resource not found"));

                    send(clientSock, fiResponse, strlen(fiResponse), 0);
                    free(fiResponse);
                    goto free_loop;
                }

                printf("[APPEND] Writing body\n");
                fwrite(req.body, sizeof(char), strlen(req.body), file);
                fclose(file);

                req.body = reDy(req.body, 2);
                req.body[0] = '\0';
                req.size = 0;
            }

            /* UNKNOWN */
            else
            {
                printf("[ERROR] Unknown request type: %s\n", req.type);
                req.size = 0;
                char *fiResponse = formingResponse(&req, 400, "Bad request");
                printf("[SEND] 400 Bad request\n");
                send(clientSock, fiResponse, strlen(fiResponse), 0);
                free(fiResponse);
                goto free_loop;
            }

            printf("[SEND] Sending 200 OK\n");
            char *fiResponse = formingResponse(&req, 200, "OK");
            send(clientSock, fiResponse, strlen(fiResponse), 0);
            free(fiResponse);

        free_loop:
            printf("[FREE] Free request fields\n");
            free(buf);
            free(req.type);
            free(req.path);
            free(req.protocol);
            free(req.agent);
            free(req.body);
        }

        /* Unsupported protocol */
        else
        {
            printf("[PROTO] Unsupported: %s\n", req.protocol);

            free(buf);
            free(req.agent);

            req.size = 0;
            strcpy(req.protocol, "TTP/1.0");

            char *fiResponse = formingResponse(&req, 405, "This Protocol version is not supported");
            send(clientSock, fiResponse, strlen(fiResponse), 0);

            printf("[SEND] 405\n");

            free(fiResponse);
            free(req.type);
            free(req.path);
            free(req.protocol);
            free(req.body);
            break;
        }
    }

    printf("[THREAD] Closing client socket %d\n", clientSock);

    pthread_mutex_lock(&mutex);
    clientCount--;
    pthread_mutex_unlock(&mutex);

    close(clientSock);

    printf("[THREAD] Exit\n");
    return NULL;
}

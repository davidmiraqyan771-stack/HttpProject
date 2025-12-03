#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "../utils/utils.h"

#define ACTIVE_USER_COUNT 10
#define PORT 8050

int clientCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *clientThread(void *arg);

int main(void)
{

    int servSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (servSocket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(servSocket, (struct sockaddr *)&server, sizeof(server)))
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(servSocket, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }


    while (1)
    {
        if (clientCount < ACTIVE_USER_COUNT)
        {
            int clientSock = accept(servSocket, NULL, NULL);
            printf("New Client: %d\n", clientSock - 3);
            pthread_mutex_lock(&mutex);
            clientCount++;
            pthread_mutex_unlock(&mutex);
            int *s = creDy(1, sizeof(int));
            *s = clientSock;

            pthread_t thr;
            pthread_create(&thr, NULL, clientThread, s);
            pthread_detach(thr);
        }
    }
}

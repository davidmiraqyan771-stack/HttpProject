#include <stdio.h>
#include <unistd.h>
#include "../utils/utils.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PROTOCOL "TTP/1.0"
#define USER_AGENT "TTP/1.0/CLIENT"
#define PORT 8050
#define ADDR "127.0.0.1"
char *reciveTheText(size_t startSize);
char *formingRequest(char *request);

#define START_SIZE 20

char append = 0;

int main(void)
{

    struct sockaddr_in client;

    client.sin_family = AF_INET;
    client.sin_port = htons(PORT);
    inet_pton(AF_INET, ADDR, &client.sin_addr.s_addr);

    int sockFd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockFd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockFd, (struct sockaddr *)&client, sizeof(client)) == -1)
    {
        perror("connect");
        close(sockFd);
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        printf("Please write the request TYPE (GET, POST, ECHO): ");
        char *type = reciveTheText(START_SIZE);
        char *path;
        char *body;
        char *request;
        printf("Please write the request PATH (Please start with /): ");
        path = reciveTheText(START_SIZE);
        if (strcmp(type, "GET") == 0 || strcmp(type + 1, "GET") == 0)
        {
            body = (char *)creDy(1, sizeof(char));
            size_t size = strlen(type) + strlen(path) + 4;
            request = (char *)creDy(size, sizeof(char));
            request = strcat(request, type);
            request = strcat(request, " ");
            request = strcat(request, path);
            request = strcat(request, "\n");
        }
        else if (strcmp(type, "POST") == 0 || strcmp(type + 1, "POST") == 0)
        {
            printf("Do you want to append (write yes or no)?\n");
            body = reciveTheText(START_SIZE);
            if (strcmp(body, "yes") == 0 || strcmp(body + 1, "yes") == 0)
            {
                append = 1;
            }
            else
            {
                append = 0;
            }
            free(body);
            printf("Please write the request BODY:\n");
            body = reciveTheText(START_SIZE);
            size_t size = strlen(type) + strlen(path) + strlen(body) + 4;
            request = (char *)creDy(size, sizeof(char));
            request = strcat(request, type);
            request = strcat(request, " ");
            request = strcat(request, path);
            request = strcat(request, "\n");
            request = strcat(request, body);
        }
        else if (strcmp(type, "ECHO") == 0 || strcmp(type + 1, "ECHO") == 0)
        {
            printf("Please write the request BODY:\n");
            body = reciveTheText(START_SIZE);
            size_t size = strlen(type) + strlen(path) + strlen(body) + 4;
            request = (char *)creDy(size, sizeof(char));
            request = strcat(request, type);
            request = strcat(request, " ");
            request = strcat(request, path);
            request = strcat(request, "\n");
            request = strcat(request, body);
        }
        else
        {
            printf("Invalid Request type\n");
            free(type);
            free(path);
            continue;
        }
        free(type);
        free(path);
        free(body);
        char *fiRequest = formingRequest(request);
        send(sockFd, fiRequest, strlen(fiRequest), 0);
        send(sockFd, "\0", 1, 0);
        free(request);
        if (strstr(fiRequest, "EXIT"))
        {
            free(fiRequest);
            break;
        }
        free(fiRequest);
        char *servRespone = reciveTheReqOrRes(START_SIZE, sockFd);
        printf("\n%s\n", servRespone);
        free(servRespone);
    }

    close(sockFd);
}

char *reciveTheText(size_t startSize)
{
    int readedValues = 0;
    char charcter;
    char *buf = (char *)creDy(startSize, sizeof(char));

    while ((charcter = getchar()) != EOF && charcter != ']')
    {
        if ((readedValues + 1) >= startSize - 1)
        {
            startSize *= 2;
            buf = reDy(buf, startSize);
        }
        buf[readedValues++] = charcter;
    }
    if (strcspn(buf, "\n") != strlen(buf))
    {
        buf[strcspn(buf, "\n")] = '\1';
    }

    buf[readedValues] = '\0';

    return buf;
}

char *formingRequest(char *request)
{
    char *body = strchr(request, '\n');
    *body++ = '\0';
    body[strlen(body)] = '\0';

    char *finishedRequest = (char *)creDy((strlen(request) + strlen(body)) * 2 + 300, sizeof(char));

    sprintf(finishedRequest, "%s %s\r\n", request, PROTOCOL);
    sprintf(finishedRequest + strlen(finishedRequest), "User-Agent: %s\r\n", USER_AGENT);
    sprintf(finishedRequest + strlen(finishedRequest), "Body-Size: %ld\r\n", strlen(body) - 1);
    sprintf(finishedRequest + strlen(finishedRequest), "Time: %ld\r\n", time(NULL));
    sprintf(finishedRequest + strlen(finishedRequest), "Append: %d\r\n\r\n\1", append);
    sprintf(finishedRequest + strlen(finishedRequest), "%s", body);
    return finishedRequest;
}

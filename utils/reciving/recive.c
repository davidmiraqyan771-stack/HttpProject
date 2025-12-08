#include <stdio.h>
#include <sys/socket.h>
#include "../utils.h"
#include <string.h>
#include <unistd.h>

char *reciveTheReqOrRes(size_t startSize, int clientSock)
{
    char *mainBuf = (char *)creDy(startSize, sizeof(char));
    char *buf1 = (char *)creDy(startSize, sizeof(char));
    size_t learnedText = 0;
    size_t previousLearnedText;
    previousLearnedText = learnedText;
    while (1)
    {
        if (learnedText + 20 >= startSize)
        {
            startSize *= 2;
            buf1 = reDy(buf1, startSize);
            mainBuf = reDy(mainBuf, startSize);
        }
        int n = recv(clientSock, buf1, 20, 0);
        if (n <= 0)
        {
            return NULL;
        }
        buf1[n] = '\0';
        learnedText += n;
        if (learnedText - previousLearnedText <= 0 || n < 20)
        {
            strcat(mainBuf, buf1);
            break;
        }

        strcat(mainBuf, buf1);

        if (learnedText == previousLearnedText)
        {
            close(clientSock);
            free(buf1);
            free(mainBuf);
            exit(EXIT_FAILURE);
        }
        previousLearnedText = learnedText;
    }
    free(buf1);
    return mainBuf;
}

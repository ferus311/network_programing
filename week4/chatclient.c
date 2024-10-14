#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define SERV_PORT 1255

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char sendline[MAXLINE], recvline[MAXLINE + 1];
    socklen_t len;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(SERV_PORT);

    while (fgets(sendline, MAXLINE, stdin) != NULL)
    {
        sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, recvline, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        recvline[n] = '\0';
        printf("Receive from: %s:%d: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), recvline);
    }

    close(sockfd);
    return 0;
}

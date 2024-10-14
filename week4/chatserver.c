#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct
{
    struct sockaddr_in addr;
    int active;
} Client;

Client clients[5];

void add_client(struct sockaddr_in *cliaddr)
{
    for (int i = 0; i < 5; i++)
    {
        if (clients[i].active && memcmp(&clients[i].addr, cliaddr, sizeof(struct sockaddr_in)) == 0)
        {
            return;
        }
    }

    for (int i = 0; i < 5; i++)
    {
        if (!clients[i].active)
        {
            clients[i].addr = *cliaddr;
            clients[i].active = 1;
            break;
        }
    }
}

void forward_message(int sockfd, char *mesg, int n, struct sockaddr_in *sender)
{
    for (int i = 0; i < 5; i++)
    {
        if (clients[i].active && memcmp(&clients[i].addr, sender, sizeof(struct sockaddr_in)) != 0)
        {
            sendto(sockfd, mesg, n, 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
        }
    }
}

#define SERV_PORT 1255
#define MAXLINE 255

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char mesg[MAXLINE];
    socklen_t len;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0)
    {
        printf("Server is running at port %d\n", SERV_PORT);
    }
    else
    {
        perror("bind failed");
        return 0;
    }

    for (;;)
    {
        len = sizeof(cliaddr);
        printf("Receiving data ...\n");
        n = recvfrom(sockfd, mesg, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        mesg[n] = '\0';
        // printf("Receive from: %s:%d: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), mesg);
        // sendto(sockfd, mesg, n, 0, (struct sockaddr *) &cliaddr, len);
        printf("Receive from: %s:%d: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), mesg);

        add_client(&cliaddr);
        forward_message(sockfd, mesg, n, &cliaddr);
    }

    close(sockfd);
    return 0;
}

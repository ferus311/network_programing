#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define MAXLINE 4096   /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8      /*maximum number of client connections*/

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

int main(int argc, char **argv)
{
    int listenfd, connfd, n;
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;
    int sockfd;
    char mesg[MAXLINE];

    // Create a socket for the soclet
    // If sockfd<0 there was an error in the creation of the socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problem in creating the socket");
        exit(2);
    }

    // preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    // bind the socket
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // listen to the socket by creating a connection queue, then wait for clients
    listen(listenfd, LISTENQ);

    printf("%s\n", "Server running...waiting for connections.");

    for (;;)
    {

        clilen = sizeof(cliaddr);
        // accept a connection
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        printf("%s\n", "Received request...");

        if ((childpid = fork()) == 0)
        { // if it’s 0, it’s child process

            printf("%s\n", "Child created for dealing with client requests");

            // close listening socket
            close(listenfd);

            while ((n = recv(connfd, buf, MAXLINE, 0)) > 0)
            {
                // printf("%s", "String received from and resent to the client:");
                // puts(buf);
                // send(connfd, buf, n, 0);
                printf("Receive from: %s:%d: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), mesg);

                add_client(&cliaddr);
                forward_message(sockfd, mesg, n, &cliaddr);
            }

            if (n < 0)
                printf("%s\n", "Read error");
            exit(0);
        }
        // close socket of the server
        close(connfd);
    }
}

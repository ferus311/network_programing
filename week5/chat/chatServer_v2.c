#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAXLINE 4096   /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8      /*maximum number of client connections*/

void initialize_server(int *listenfd, struct sockaddr_in *servaddr);
void accept_new_connection(int listenfd, int *client, fd_set *allset, int *maxfd);
void handle_client_data(int sockfd, int *client, fd_set *allset, int maxfd, int *nready);

int main(int argc, char **argv)
{
    int listenfd, connfd, sockfd, nready, maxfd, client[FD_SETSIZE];
    printf("so luong : %d\n", FD_SETSIZE);
    fd_set rset, allset;
    struct sockaddr_in servaddr;

    initialize_server(&listenfd, &servaddr);

    maxfd = listenfd; // Initialize
    for (int i = 0; i < FD_SETSIZE; i++)
        client[i] = -1; // -1 indicates available entry
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    while (1)
    {
        rset = allset; // Structure assignment
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset))
        { // New client connection
            accept_new_connection(listenfd, client, &allset, &maxfd);
            if (--nready <= 0)
                continue; // No more readable descriptors
        }

        for (int i = 0; i <= maxfd; i++)
        {
            if ((sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset))
            {
                handle_client_data(sockfd, client, &allset, maxfd, &nready);
                if (--nready <= 0)
                    break; // No more readable descriptors
            }
        }
    }

    return 0;
}

void initialize_server(int *listenfd, struct sockaddr_in *servaddr)
{
    // Create a socket for the socket
    if ((*listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problem in creating the socket");
        exit(2);
    }

    // Preparation of the socket address
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(SERV_PORT);

    // Bind the socket
    bind(*listenfd, (struct sockaddr *)servaddr, sizeof(*servaddr));

    // Listen to the socket by creating a connection queue, then wait for clients
    listen(*listenfd, LISTENQ);

    printf("%s\n", "Server running...waiting for connections.");
}

void accept_new_connection(int listenfd, int *client, fd_set *allset, int *maxfd)
{
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;

    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
    {
        perror("Problem in accepting connection");
        return;
    }

    printf("New client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

    int i;
    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (client[i] < 0)
        {
            client[i] = connfd; // Save descriptor
            break;
        }
    }

    if (i == FD_SETSIZE)
    {
        fprintf(stderr, "Too many clients\n");
        close(connfd);
        return;
    }

    FD_SET(connfd, allset); // Add new descriptor to set
    if (connfd > *maxfd)
        *maxfd = connfd; // For select
}

void handle_client_data(int sockfd, int *client, fd_set *allset, int maxfd, int *nready)
{
    ssize_t n;
    char buf[MAXLINE];

    if ((n = read(sockfd, buf, MAXLINE)) == 0)
    {
        // Connection closed by client
        close(sockfd);
        FD_CLR(sockfd, allset);
        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (client[i] == sockfd)
            {
                client[i] = -1;
                break;
            }
        }
    }
    else
    {
        // Forward message to all other clients
        buf[n] = '\0';
        printf("Received message from client %d: %s\n", sockfd, buf);
        for (int j = 0; j <= maxfd; j++)
        {
            if (client[j] != -1 && client[j] != sockfd)
            {
                if (write(client[j], buf, n) < 0)
                {
                    perror("write to client failed");
                    close(client[j]);
                    FD_CLR(client[j], allset);
                    client[j] = -1;
                }
            }
        }
    }

    (*nready)--;
}

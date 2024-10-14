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

int main(int argc, char **argv)
{
    int listenfd, connfd, sockfd, nready, maxfd, client[FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    // Create a socket for the socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problem in creating the socket");
        exit(2);
    }

    // Preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    // Bind the socket
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // Listen to the socket by creating a connection queue, then wait for clients
    listen(listenfd, LISTENQ);

    printf("%s\n", "Server running...waiting for connections.");

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
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
            {
                perror("Problem in accepting connection");
                continue;
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
                continue;
            }

            FD_SET(connfd, &allset); // Add new descriptor to set
            if (connfd > maxfd)
                maxfd = connfd; // For select

            if (--nready <= 0)
                continue; // No more readable descriptors
        }

        for (int i = 0; i <= maxfd; i++)
        {
            if ((sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset))
            {
                if ((n = read(sockfd, buf, MAXLINE)) == 0)
                {
                    // Connection closed by client
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
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
                                FD_CLR(client[j], &allset);
                                client[j] = -1;
                            }
                        }
                    }
                }

                if (--nready <= 0)
                    break; // No more readable descriptors
            }
        }
    }

    return 0;
}

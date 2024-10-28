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
#define MAX_USERS 100  /*maximum number of users*/

typedef struct {
    char username[50];
    char password[50];
} User;

User users[MAX_USERS];
int user_count = 0;

void initialize_server(int *listenfd, struct sockaddr_in *servaddr);
void accept_new_connection(int listenfd, int *client, fd_set *allset, int *maxfd);
void handle_client_data(int sockfd, int *client, fd_set *allset, int maxfd, int *nready, int *logged_in);
int check_login(const char *message);
void load_users(const char *filename);

int main(int argc, char **argv)
{
    int listenfd, connfd, sockfd, nready, maxfd, client[FD_SETSIZE];
    int logged_in[FD_SETSIZE] = {0}; // Array to track login status of clients
    fd_set rset, allset;
    struct sockaddr_in servaddr;

    load_users("users.txt");
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
                handle_client_data(sockfd, client, &allset, maxfd, &nready, logged_in);
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

void handle_client_data(int sockfd, int *client, fd_set *allset, int maxfd, int *nready, int *logged_in)
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
                logged_in[i] = 0; // Reset login status
                break;
            }
        }
    }
    else
    {
        buf[n] = '\0';
        if (!logged_in[sockfd])
        {
            if (check_login(buf))
            {
                logged_in[sockfd] = 1;
                const char *login_success = "Login successful\n";
                write(sockfd, login_success, strlen(login_success));
            }
            else
            {
                const char *login_fail = "Login failed\n";
                write(sockfd, login_fail, strlen(login_fail));
            }
        }
        else
        {
            // Forward message to all other clients
            printf("Received message from client %d: %s\n", sockfd, buf);
            for (int j = 0; j <= maxfd; j++)
            {
                if (client[j] != -1 && client[j] != sockfd && logged_in[j])
                {
                    if (write(client[j], buf, n) < 0)
                    {
                        perror("write to client failed");
                        close(client[j]);
                        FD_CLR(client[j], allset);
                        client[j] = -1;
                        logged_in[j] = 0; // Reset login status
                    }
                }
            }
        }
    }

    (*nready)--;
}

int check_login(const char *message)
{
    // Simple login check: message format "LOG_username_password"
    const char *prefix = "LOG_";
    if (strncmp(message, prefix, strlen(prefix)) == 0)
    {
        // Extract username and password
        const char *credentials = message + strlen(prefix);
        char username[50], password[50];
        if (sscanf(credentials, "%49[^_]_%49s", username, password) == 2)
        {
            // Check username and password against loaded users
            for (int i = 0; i < user_count; i++)
            {
                if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0)
                {
                    return 1; // Login successful
                }
            }
        }
    }
    return 0; // Login failed
}

void load_users(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening user file");
        exit(1);
    }

    while (fscanf(file, "%49s %49s", users[user_count].username, users[user_count].password) == 2)
    {
        user_count++;
        if (user_count >= MAX_USERS)
        {
            fprintf(stderr, "Maximum number of users reached\n");
            break;
        }
    }

    fclose(file);
}

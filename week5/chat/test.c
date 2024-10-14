#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE 4096
#define SERV_PORT 3000

typedef struct {
    struct sockaddr_in addr;
    int active;
    int sockfd;
} Client;

Client clients[5];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(struct sockaddr_in *cliaddr, int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 5; i++) {
        if (clients[i].active && memcmp(&clients[i].addr, cliaddr, sizeof(struct sockaddr_in)) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }

    for (int i = 0; i < 5; i++) {
        if (!clients[i].active) {
            clients[i].addr = *cliaddr;
            clients[i].sockfd = sockfd;
            clients[i].active = 1;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void forward_message(int sender_sockfd, char *mesg, int n) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 5; i++) {
        if (clients[i].active && clients[i].sockfd != sender_sockfd) {
            if (write(clients[i].sockfd, mesg, n) < 0) {
                perror("write to client failed");
                close(clients[i].sockfd);
                clients[i].active = 0;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int sockfd = *((int *)arg);
    free(arg);
    char buf[MAXLINE];
    int n;

    while ((n = read(sockfd, buf, MAXLINE)) > 0) {
        buf[n] = '\0';
        printf("Received message: %s\n", buf);
        forward_message(sockfd, buf, n);
    }

    close(sockfd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 5; i++) {
        if (clients[i].sockfd == sockfd) {
            clients[i].active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

int main() {
    int listenfd, *connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    pthread_t tid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    printf("Server is running at port %d\n", SERV_PORT);

    while (1) {
        clilen = sizeof(cliaddr);
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        add_client(&cliaddr, *connfd);
        pthread_create(&tid, NULL, handle_client, connfd);
    }

    close(listenfd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define SERV_PORT 3000

typedef struct {
    char username[50];
    char password[50];
    char schedule[7][256]; // Schedule for each day of the week
} User;

User users[] = {
    {"user1", "pass1", {"Math", "Physics", "Chemistry", "Biology", "History", "Geography", "Free"}},
    {"user2", "pass2", {"English", "Math", "Physics", "Chemistry", "Biology", "History", "Free"}}
};
int num_users = 2;

User* authenticate(char* username, char* password) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return &users[i];
        }
    }
    return NULL;
}

void handle_client(int connfd) {
    char buf[MAXLINE];
    char username[50], password[50], day[10];
    int n;

    // Receive username and password
    n = recv(connfd, buf, MAXLINE, 0);
    sscanf(buf, "%s %s", username, password);

    User* user = authenticate(username, password);
    if (user == NULL) {
        send(connfd, "Invalid credentials", 19, 0);
        close(connfd);
        return;
    }
    send(connfd, "Login successful", 16, 0);

    // Receive day request
    n = recv(connfd, day, sizeof(day), 0);
    day[n] = '\0';

    if (strcmp(day, "ALL") == 0) {
        for (int i = 0; i < 7; i++) {
            send(connfd, user->schedule[i], strlen(user->schedule[i]), 0);
            send(connfd, "\n", 1, 0);
        }
    } else {
        int day_index = atoi(day);
        if (day_index >= 0 && day_index < 7) {
            send(connfd, user->schedule[day_index], strlen(user->schedule[day_index]), 0);
        } else {
            send(connfd, "Invalid day", 11, 0);
        }
    }

    close(connfd);
}

int main() {
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    printf("Server running...waiting for connections.\n");

    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        handle_client(connfd);
    }

    close(listenfd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define SERV_PORT 3000

void login(int sockfd) {
    char username[50], password[50], buf[MAXLINE];

    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    snprintf(buf, sizeof(buf), "%s %s", username, password);
    send(sockfd, buf, strlen(buf), 0);

    int n = recv(sockfd, buf, MAXLINE, 0);
    buf[n] = '\0';
    printf("%s\n", buf);

    if (strcmp(buf, "Login successful") != 0) {
        exit(1);
    }
}

void request_schedule(int sockfd) {
    char day[10], buf[MAXLINE];

    printf("Enter day (0-6 for specific day, ALL for entire week): ");
    scanf("%s", day);

    send(sockfd, day, strlen(day), 0);

    int n;
    while ((n = recv(sockfd, buf, MAXLINE, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(SERV_PORT);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection error");
        exit(1);
    }

    login(sockfd);
    request_schedule(sockfd);

    close(sockfd);
    return 0;
}

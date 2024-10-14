#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

int main()
{
    int n;
    fd_set readfds;
    struct timeval tv;
    char buf1[256], buf2[256];

    struct sockaddr_in server_socket;
    server_socket.sin_family = AF_INET;
    server_socket.sin_port = htons(3000);
    server_socket.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("server IP = %s\n", inet_ntoa(server_socket.sin_addr));

    int s1 = socket(AF_INET, SOCK_STREAM, 0); // client socket

    if (connect(s1, (struct sockaddr *)&server_socket, sizeof(server_socket)) < 0) {
        printf("Error in connecting to server\n");
        return 1;
    } else {
        printf("connected to server\n");
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(s1, &readfds);

        int maxfd = s1 > STDIN_FILENO ? s1 : STDIN_FILENO;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buf1, sizeof(buf1), stdin) != NULL) {
                send(s1, buf1, strlen(buf1), 0);
            }
        }

        if (FD_ISSET(s1, &readfds)) {
            n = recv(s1, buf2, sizeof(buf2) - 1, 0);
            if (n > 0) {
                buf2[n] = '\0';
                printf("Received from server: %s\n", buf2);
            } else if (n == 0) {
                printf("Server closed connection\n");
                break;
            } else {
                perror("recv error");
                break;
            }
        }
    }

    close(s1);
    return 0;
}

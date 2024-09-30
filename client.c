#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096 /* max text line length */
#define SERV_PORT 3000 /* port */

void send_file(FILE *fp, int sockfd) {
    char sendline[MAXLINE];
    size_t n;

    while ((n = fread(sendline, sizeof(char), MAXLINE, fp)) > 0) {
        if (send(sockfd, sendline, n, 0) == -1) {
            perror("Error sending file");
            exit(3);
        }
    }
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(1);
    }

    char *file_path = argv[1];
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        exit(2);
    }

    // creation of the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.109"); // Use the correct server IP address
    servaddr.sin_port = htons(SERV_PORT);

    // connect to the server
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Connection error");
        exit(3);
    }

    send_file(fp, sockfd);
    printf("File '%s' sent to server\n", file_path);

    fclose(fp);
    close(sockfd);
    return 0;
}

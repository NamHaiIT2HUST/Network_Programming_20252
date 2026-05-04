#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Syntax: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }

    int my_port = atoi(argv[1]);
    char *peer_ip = argv[2];
    int peer_port = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in peer_addr = {0};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    peer_addr.sin_addr.s_addr = inet_addr(peer_ip);

    struct pollfd fds[2];
    
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    char buf[1024];

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            perror("poll failed");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) != NULL) {
                buf[strcspn(buf, "\r\n")] = 0; 
                
                if (strlen(buf) > 0) {
                    sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
                }
            }
        }

        if (fds[1].revents & POLLIN) {
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            
            int n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
            if (n > 0) {
                buf[n] = 0;
                printf("\r[Peer]: %s\n", buf);
            }
        }
    }

    close(sockfd);
    return 0;
}
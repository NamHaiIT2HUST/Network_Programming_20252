#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Syntax: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket failed");
        return 1;
    }

    unsigned long ul = 1;
    ioctl(sock, FIONBIO, &ul);
    ioctl(STDIN_FILENO, FIONBIO, &ul); 

    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port_s);

    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip_d);
    dest_addr.sin_port = htons(port_d);

    printf("--- UDP Chat Ready ---\n");
    printf("Listening on port: %d\n", port_s);
    printf("Sending to: %s:%d\n\n", ip_d, port_d);

    char buf[1024];

    while (1) {
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from_addr, &from_len);
        if (n > 0) {
            buf[n] = 0;
            printf("\r[Friend]: %s\n", buf);
        }

        int m = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (m > 0) {
            buf[m] = 0;
            buf[strcspn(buf, "\r\n")] = 0; 
            
            if (strstr(buf, "exit") == buf) {
                printf("Exiting chat...\n");
                break;
            }

            if (strlen(buf) > 0) {
                sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                printf("\r[You]: %s\n", buf);
            }
        }

        usleep(10000); 
    }

    close(sock);
    return 0;
}
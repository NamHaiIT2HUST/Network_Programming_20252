#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Syntax Error. Enter: %s <IP_Address> <Port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    printf("UDP Client Ready. Sending to %s:%d\n\n", server_ip, server_port);

    char input[1024];
    char recv_buf[1024];

    while (1) {
        printf("Enter text (or 'exit'): ");
        memset(input, 0, sizeof(input));
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        input[strcspn(input, "\n")] = 0; 
        
        if (strstr(input, "exit") != NULL) {
            printf("Exiting...\n");
            break;
        }

        sendto(client, input, strlen(input), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        memset(recv_buf, 0, sizeof(recv_buf));
        
        int n = recvfrom(client, recv_buf, sizeof(recv_buf) - 1, 0, (struct sockaddr *)&from_addr, &from_len);
        if (n > 0) {
            printf("  => [Echo from Server]: %s\n\n", recv_buf);
        }
    }

    close(client);
    return 0;
}
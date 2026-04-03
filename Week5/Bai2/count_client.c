#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Syntax Error. Enter: %s <IP_Address> <Port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_ip);
    addr.sin_port = htons(server_port);

    printf("Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        close(client);
        return 1;
    }
    printf("Connected successfully!\n\n");

    char input[1024];
    while (1) {
        printf("Enter text (or 'exit'): ");
        memset(input, 0, sizeof(input));
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        input[strcspn(input, "\n")] = 0; 
        
        if (strstr(input, "exit") != NULL) {
            printf("Disconnecting...\n");
            break;
        }

        int bytes_sent = send(client, input, strlen(input), 0);
        if (bytes_sent < 0) {
            perror("send failed");
            break;
        }
    }

    close(client);
    return 0;
}
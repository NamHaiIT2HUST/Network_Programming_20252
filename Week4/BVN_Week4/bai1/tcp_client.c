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
        printf("Example: %s 127.0.0.1 9000\n", argv[0]);
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

    printf("Connecting %s:%d...\n", server_ip, server_port);
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        close(client);
        return 1;
    }
    printf("Successfully! Enter data (Go 'exit' out):\n");

    char buf[1024];
    while (1) {
        printf(">> ");
        memset(buf, 0, sizeof(buf));
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            break; 
        }

        if (strstr(buf, "exit") != NULL) {
            printf("Disconnecting...\n");
            break;
        }

        int bytes_sent = send(client, buf, strlen(buf), 0);
        if (bytes_sent < 0) {
            perror("Fail");
            break;
        }
    }

    close(client);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

    printf("Dang ket noi den %s:%d...\n", server_ip, server_port);
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        close(client);
        return 1;
    }
    printf("Ket noi thanh cong!\n\n");

    char buf[1024];

    while (1) {
        memset(buf, 0, sizeof(buf));
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        
        if (n <= 0) {
            printf("\nServer da ngat ket noi.\n");
            break;
        }

        printf("%s", buf);
        fflush(stdout); 

        if (strstr(buf, "Email DHBK") != NULL) {
            break; 
        }

        memset(buf, 0, sizeof(buf));
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            send(client, buf, strlen(buf), 0);
        }
    }

    close(client);
    return 0;
}
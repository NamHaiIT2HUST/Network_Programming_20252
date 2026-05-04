#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax Error. Enter: %s <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listener < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    printf("[Server] UDP Echo Server is running on port %d...\n\n", port);

    char buf[1024];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        memset(buf, 0, sizeof(buf));

        int n = recvfrom(listener, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (n > 0) {
            printf("[Server] Received from %s:%d => %s\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port), 
                   buf);
            
            sendto(listener, buf, n, 0, (struct sockaddr *)&client_addr, client_addr_len);
        }
    }

    close(listener);
    return 0;
}
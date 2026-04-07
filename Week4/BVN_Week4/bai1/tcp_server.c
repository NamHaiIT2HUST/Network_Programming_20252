#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax Error. Enter: %s <Port>\n", argv[0]);
        printf("Example: %s 9000\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int opt = 1; 
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }

    int ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind() failed");
        exit(1);
    }

    ret = listen(listener, 5);
    if (ret < 0) {
        perror("listen() failed");
        exit(1);
    }

    while (1) {
        printf("[Server] Listening on port %d...\n", port);
        
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0) {
            perror("accept() failed");
            continue;
        }

        printf("[Server] Connected from: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char buf[1024];
        while (1) {
            memset(buf, 0, sizeof(buf)); 
            int n = recv(client, buf, sizeof(buf) - 1, 0);
            
            if (n <= 0) {
                printf("[Server] Client disconnected.\n");
                printf("[Server] Continuing to listen...\n\n");
                break;
            }

            buf[strcspn(buf, "\n")] = 0; 
            buf[strcspn(buf, "\r")] = 0;
            
            if (strlen(buf) > 0) {
                printf("[Received from Client]: %s\n", buf);
            }
        }

        close(client); 
    }

    close(listener);
    return 0;
}
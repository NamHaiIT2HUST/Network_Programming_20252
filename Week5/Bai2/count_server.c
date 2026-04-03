#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax Error. Enter: %s <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    int opt = 1; 
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    listen(listener, 5);

    while (1) {
        printf("[Server] Listening on port %d...\n", port);
        
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;
        printf("[Server] A new client connected.\n\n");

        char buf[1024];
        int total_count = 0;
        int state = 0;

        while (1) {
            memset(buf, 0, sizeof(buf));
            int n = recv(client, buf, sizeof(buf) - 1, 0);
            
            if (n <= 0) {
                printf("\n[Server] Client disconnected.\n\n");
                break;
            }

            for (int i = 0; i < n; i++) {
                if (buf[i] == '0' + state) {
                    state++;
                    if (state == 10) {
                        total_count++;
                        state = 0;
                    }
                } else {
                    if (buf[i] == '0') {
                        state = 1;
                    } else {
                        state = 0;
                    }
                }
            }

            printf("[Server] Data received. Current occurrences of '0123456789': %d\n", total_count);
        }
        close(client);
    }

    close(listener);
    return 0;
}
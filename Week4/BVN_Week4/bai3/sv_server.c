#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

struct SinhVien {
    char mssv[20];
    char hoten[64];
    char ngaysinh[20];
    float dtb;
};

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
        perror("bind() failed");
        return 1;
    }

    listen(listener, 5);

    while (1) {
        printf("[Server] Listening on port %d...\n", port);
        
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            perror("accept() failed");
            continue;
        }
        printf("[Server] A new client connected.\n\n");

        struct SinhVien sv;
        while (1) {
            int n = recv(client, &sv, sizeof(sv), 0);
            
            if (n <= 0) {
                printf("[Server] Client disconnected.\n\n");
                break;
            }

            printf("--- Received Student Information ---\n");
            printf("Student ID: %s\n", sv.mssv);
            printf("Full Name: %s\n", sv.hoten);
            printf("Date of Birth: %s\n", sv.ngaysinh);
            printf("GPA: %.2f\n", sv.dtb);
            printf("------------------------------------\n\n");
        }
        close(client);
    }

    close(listener);
    return 0;
}
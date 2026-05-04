#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

struct SinhVien {
    char mssv[20];
    char hoten[64];
    char ngaysinh[20];
    float dtb;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Syntax Error. Enter: %s <Port> <Log_File>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_file = argv[2];

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

    FILE *f = fopen(log_file, "a");
    if (f == NULL) {
        perror("Cannot open log file");
        return 1;
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

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("[Server] A new client connected from: %s\n\n", client_ip);

        struct SinhVien sv;
        while (1) {
            int n = recv(client, &sv, sizeof(sv), 0);
            
            if (n <= 0) {
                printf("[Server] Client disconnected.\n\n");
                break;
            }

            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);

            printf("--- Received Student Information ---\n");
            printf("Student ID: %s\n", sv.mssv);
            printf("Full Name: %s\n", sv.hoten);
            printf("Date of Birth: %s\n", sv.ngaysinh);
            printf("GPA: %.2f\n", sv.dtb);
            printf("------------------------------------\n\n");

            fprintf(f, "%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.hoten, sv.ngaysinh, sv.dtb);
            fflush(f); 
        }
        close(client);
    }

    fclose(f);
    close(listener);
    return 0;
}
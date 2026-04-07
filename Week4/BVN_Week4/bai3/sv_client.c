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

int check_mssv(char *str) {
    int len = strlen(str);
    if (len != 8 && len != 9) return 0; 
    
    for (int i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') return 0; 
    }
    return 1; 
}

int check_date(char *str) {
    if (strlen(str) != 10) return 0;
    if (str[2] != '/' || str[5] != '/') return 0;
    
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    
    int d, m, y;
    sscanf(str, "%d/%d/%d", &d, &m, &y);
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > 31) return 0;
    
    if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 30) return 0;
    if (m == 2) {
        int is_leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
        if (d > 28 + is_leap) return 0;
    }
    
    return 1; 
}

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

    struct SinhVien sv;
    char input[256];

    while (1) {
        printf("--- Enter Student Information ---\n");
        memset(&sv, 0, sizeof(sv)); 
        int is_exit = 0;

        while (1) {
            printf("Student ID (8-9 digits, or 'exit'): ");
            memset(input, 0, sizeof(input));
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0; 
            
            // Đã sửa thành strstr để chống lỗi kẹt phím mũi tên
            if (strstr(input, "exit") != NULL) {
                is_exit = 1;
                break;
            }
            
            if (check_mssv(input)) {
                strcpy(sv.mssv, input);
                break; 
            } else {
                printf("  [!] ERROR: ID must be 8 or 9 digits. Try again!\n");
            }
        }

        if (is_exit) {
            printf("Disconnecting...\n");
            break;
        }

        printf("Full Name: ");
        fgets(sv.hoten, sizeof(sv.hoten), stdin);
        sv.hoten[strcspn(sv.hoten, "\n")] = 0;

        while (1) {
            printf("Date of Birth (DD/MM/YYYY): ");
            fgets(sv.ngaysinh, sizeof(sv.ngaysinh), stdin);
            sv.ngaysinh[strcspn(sv.ngaysinh, "\n")] = 0;
            
            if (check_date(sv.ngaysinh)) {
                break; 
            } else {
                printf("  [!] ERROR: Invalid date format. Try again!\n");
            }
        }

        printf("GPA: ");
        memset(input, 0, sizeof(input));
        fgets(input, sizeof(input), stdin);
        sv.dtb = atof(input); 

        int bytes_sent;
        bytes_sent = send(client, &sv, sizeof(sv), 0);
        if (bytes_sent < 0) {
            perror("send failed");
            break;
        }
        printf("=> Student information sent successfully!\n\n");
    }

    close(client);
    return 0;
}
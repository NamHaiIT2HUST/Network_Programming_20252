#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Luong (Thread) xu ly doc lap cho tung Client
void *client_handler(void *arg) {
    int client = *(int *)arg;
    free(arg); // Giai phong bo nho cap phat dong chong Race Condition

    char buf[256];
    char *prompt = "Giao thuc Time Server. Hay nhap: GET_TIME [format]\n";
    send(client, prompt, strlen(prompt), 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0) break; // Client ngat ket noi
        
        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0; // Xoa Enter
        
        if (strlen(buf) == 0) continue;
        if (strcmp(buf, "exit") == 0) break;

        char response[256];

        // Kiem tra cu phap lenh GET_TIME (9 ky tu)
        if (strncmp(buf, "GET_TIME ", 9) == 0) {
            char *format_str = buf + 9; // Cat lay phan format
            
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char time_buf[64];

            // Kiem tra Format va ep kieu thoi gian
            if (strcmp(format_str, "dd/mm/yyyy") == 0) {
                strftime(time_buf, sizeof(time_buf), "%d/%m/%Y", tm_info);
                sprintf(response, "Thoi gian: %s\n", time_buf);
            } 
            else if (strcmp(format_str, "dd/mm/yy") == 0) {
                strftime(time_buf, sizeof(time_buf), "%d/%m/%y", tm_info);
                sprintf(response, "Thoi gian: %s\n", time_buf);
            } 
            else if (strcmp(format_str, "mm/dd/yyyy") == 0) {
                strftime(time_buf, sizeof(time_buf), "%m/%d/%Y", tm_info);
                sprintf(response, "Thoi gian: %s\n", time_buf);
            } 
            else if (strcmp(format_str, "mm/dd/yy") == 0) {
                strftime(time_buf, sizeof(time_buf), "%m/%d/%y", tm_info);
                sprintf(response, "Thoi gian: %s\n", time_buf);
            } 
            else {
                sprintf(response, "Loi: Format khong ho tro. Vui long dung: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n");
            }
        } else {
            sprintf(response, "Loi: Sai cu phap lenh. Dung chuan: GET_TIME [format]\n");
        }
        
        send(client, response, strlen(response), 0);
    }
    
    printf("Client %d da ngat ket noi.\n", client);
    close(client);
    pthread_exit(NULL); // Ket thuc luong an toan
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);
    
    printf("Time Server (Multithreading) dang chay tren cong 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        printf("Client moi ket noi: %d\n", client);

        // Cap phat dong de an toan cho luong
        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, (void *)new_sock);
        pthread_detach(tid); // Tu dong don rac khi luong chay xong
    }

    close(listener);
    return 0;
}
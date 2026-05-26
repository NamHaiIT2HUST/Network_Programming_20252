#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }
    
    listen(listener, 10);
    printf("Time Server (Multiprocess) is listening on port 9000...\n");

    signal(SIGCHLD, signal_handler);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        printf("New client connected: %d\n", client);

        if (fork() == 0) { 
            close(listener); 
            char buf[256];

            char *prompt = "Giao thuc Time Server. Hay nhap: GET_TIME [format]\n";
            send(client, prompt, strlen(prompt), 0);

            while (1) {
                int len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) break; 
                
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0; 
                
                if (strlen(buf) == 0) continue;
                if (strcmp(buf, "exit") == 0) break;

                char response[256];

                if (strncmp(buf, "GET_TIME ", 9) == 0) {
                    
                    char *format_str = buf + 9; 

                    time_t t = time(NULL);
                    struct tm *tm_info = localtime(&t);
                    char time_buf[64];

                    // Kiểm tra và khớp Format
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
            
            printf("Client %d disconnected.\n", client);
            close(client);
            exit(0); 
        }

        close(client);
    }

    close(listener);
    return 0;
}
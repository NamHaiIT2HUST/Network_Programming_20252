#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax: %s <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Server is listening on port %d...\n", port);
    
    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    struct timeval tv;
    char buf[512];

    int states[FD_SETSIZE];
    memset(states, 0, sizeof(states));
    char client_ids[FD_SETSIZE][64];
    memset(client_ids, 0, sizeof(client_ids));

    while (1) {
        fdtest = fdread;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select() failed");
            break;
        }
        if (ret == 0) {
            continue; 
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE) {
                        printf("New client connected %d\n", client);
                        FD_SET(client, &fdread);
                        
                        states[client] = 1; 
                        char *msg = "Vui long nhap cu phap (client_id: client_name):\n";
                        send(client, msg, strlen(msg), 0);
                    } else {
                        close(client);
                    }
                } else {
                    ret = recv(i, buf, sizeof(buf) - 1, 0);
                    if (ret <= 0) {
                        printf("Client %d disconnected\n", i);    
                        FD_CLR(i, &fdread);
                        states[i] = 0; 
                    } else {
                        buf[ret] = 0;
                        buf[strcspn(buf, "\r\n")] = 0;
                        
                        if (strlen(buf) == 0) continue;

                        if (states[i] == 1) {
                            char id[64], name[64];
                            if (sscanf(buf, "%63[^:]: %63s", id, name) == 2) {
                                strcpy(client_ids[i], id);
                                states[i] = 2;
                                
                                char *ok_msg = "Dang nhap thanh cong! Ban da co the gui tin nhan.\n";
                                send(i, ok_msg, strlen(ok_msg), 0);
                            } else {
                                char *err_msg = "Sai cu phap! Yeu cau nhap (client_id: client_name):\n";
                                send(i, err_msg, strlen(err_msg), 0);
                            }
                        } 
                        else if (states[i] == 2) {
                            time_t t = time(NULL);
                            struct tm *tm_info = localtime(&t);
                            char time_buf[64];
                            strftime(time_buf, sizeof(time_buf), "%Y/%m/%d %I:%M:%S%p", tm_info);

                            char send_buf[1024];
                            snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n", time_buf, client_ids[i], buf);

                            for (int j = 0; j < FD_SETSIZE; j++) {
                                if (FD_ISSET(j, &fdread) && j != listener && j != i && states[j] == 2) {
                                    send(j, send_buf, strlen(send_buf), 0);
                                }
                            }
                            printf("Broadcasted: %s", send_buf);
                        }
                    }
                }            
            }
        }
    }

    close(listener);
    return 0;
}
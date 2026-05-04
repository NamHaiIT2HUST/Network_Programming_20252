#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_FDS 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax: %s <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind failed");
        return 1;
    }
    
    listen(listener, 10);
    printf("Server is listening on port %d...\n", port);
    
    struct pollfd fds[MAX_FDS];
    for (int i = 0; i < MAX_FDS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    
    fds[0].fd = listener;
    int num_clients = 0;
    char buf[1024];

    while (1) {
        int ret = poll(fds, MAX_FDS, -1);
        if (ret < 0) {
            perror("poll failed");
            break;
        }

        for (int i = 0; i < MAX_FDS; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client != -1) {
                        int j;
                        for (j = 0; j < MAX_FDS; j++) {
                            if (fds[j].fd == -1) {
                                fds[j].fd = client;
                                fds[j].events = POLLIN;
                                num_clients++;
                                
                                char msg[256];
                                sprintf(msg, "Xin chao. Hien co %d clients dang ket noi.\n", num_clients);
                                send(client, msg, strlen(msg), 0);
                                
                                printf("New client %d connected. Total: %d\n", client, num_clients);
                                break;
                            }
                        }
                        if (j == MAX_FDS) {
                            close(client);
                        }
                    }
                } else {
                    int client_fd = fds[i].fd;
                    ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
                    
                    if (ret <= 0) {
                        printf("Client %d disconnected\n", client_fd);    
                        close(client_fd);
                        fds[i].fd = -1;
                        num_clients--;
                    } else {
                        buf[ret] = 0;
                        
                        char temp[1024];
                        strcpy(temp, buf);
                        temp[strcspn(temp, "\r\n")] = 0;
                        
                        if (strcmp(temp, "exit") == 0) {
                            char *bye = "Chao tam biet!\n";
                            send(client_fd, bye, strlen(bye), 0);
                            close(client_fd);
                            fds[i].fd = -1;
                            num_clients--;
                            printf("Client %d exited. Total: %d\n", client_fd, num_clients);
                        } else {
                            for (int k = 0; k < ret; k++) {
                                if (buf[k] >= 'a' && buf[k] < 'z') buf[k]++;
                                else if (buf[k] == 'z') buf[k] = 'a';
                                else if (buf[k] >= 'A' && buf[k] < 'Z') buf[k]++;
                                else if (buf[k] == 'Z') buf[k] = 'A';
                                else if (buf[k] >= '0' && buf[k] <= '9') buf[k] = '9' - (buf[k] - '0');
                            }
                            send(client_fd, buf, ret, 0);
                        }
                    }
                }            
            }
        }
    }

    close(listener);
    return 0;
}
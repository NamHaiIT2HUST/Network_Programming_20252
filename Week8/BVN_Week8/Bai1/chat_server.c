#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>

#define MAX_FDS 1024

int main() {
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
    addr.sin_port = htons(8080);
    
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
    
    printf("Server is listening on port 8080...\n");

    struct pollfd fds[MAX_FDS];
    for (int i = 0; i < MAX_FDS; i++) {
        fds[i].fd = -1; 
        fds[i].events = POLLIN;
    }

    fds[0].fd = listener;

    char buf[256];
    char *ids[MAX_FDS] = {NULL};

    while (1) {
        int ret = poll(fds, MAX_FDS, -1);
        if (ret < 0) {
            perror("poll() failed");
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
                                printf("New client connected %d\n", client);
                                char *msg = "Xin chao. Hay dang nhap!\n";
                                send(client, msg, strlen(msg), 0);
                                break;
                            }
                        }
                        if (j == MAX_FDS) {
                            printf("Server full. Cannot accept %d\n", client);
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
                        
                        free(ids[client_fd]);
                        ids[client_fd] = NULL;
                    } else {
                        buf[ret] = 0;
                        printf("Received from %d: %s\n", client_fd, buf);

                        if (ids[client_fd] == NULL) {
                            char cmd[32], id[32], tmp[32];
                            int n = sscanf(buf, "%s%s%s", cmd, id, tmp);
                            if (n != 2) {
                                char *msg = "Error. Thua hoac thieu tham so!\n";
                                send(client_fd, msg, strlen(msg), 0);
                            } else {
                                if (strcmp(cmd, "client_id:") != 0) {
                                    char *msg = "Error. Sai cu phap!\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                } else {
                                    char *msg = "OK. Hay nhap tin nhan!\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                    
                                    ids[client_fd] = malloc(strlen(id) + 1);
                                    strcpy(ids[client_fd], id);
                                }
                            }
                        } else {
                            char target[32];
                            int n = sscanf(buf, "%s", target);
                            if (n == 0) continue;
                            
                            if (strcmp(target, "all") == 0) {
                                for (int j = 0; j < MAX_FDS; j++) {
                                    if (ids[j] != NULL && client_fd != j) {
                                        send(j, ids[client_fd], strlen(ids[client_fd]), 0);
                                        send(j, ": ", 2, 0);
                                        char *pos = buf + strlen(target) + 1;
                                        send(j, pos, strlen(pos), 0);
                                    }
                                }
                            } else {
                                int j = 0;
                                for (; j < MAX_FDS; j++) {
                                    if (ids[j] != NULL && strcmp(target, ids[j]) == 0) break;
                                }
                                
                                if (j < MAX_FDS) {
                                    send(j, ids[client_fd], strlen(ids[client_fd]), 0);
                                    send(j, ": ", 2, 0);
                                    char *pos = buf + strlen(target) + 1;
                                    send(j, pos, strlen(pos), 0);
                                }
                            }                                  
                        }
                    }
                }            
            }
        }
    }

    close(listener);
    return 0;
}
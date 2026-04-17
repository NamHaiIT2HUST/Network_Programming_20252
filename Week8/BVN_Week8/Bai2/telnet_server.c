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
    int login[MAX_FDS] = {0};

    while (1) {
        int ret = poll(fds, MAX_FDS, 60000);
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        if (ret == 0) {
            printf("Timed out.\n");
            continue;
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
                                break;
                            }
                        }
                        if (j == MAX_FDS) {
                            close(client);
                        }
                    }
                } else {
                    int client_fd = fds[i].fd;
                    ret = recv(client_fd, buf, sizeof(buf), 0);
                    if (ret <= 0) {
                        printf("Client %d disconnected\n", client_fd);    
                        close(client_fd);
                        fds[i].fd = -1;
                        login[client_fd] = 0;
                    } else {
                        buf[ret] = 0;
                        if (buf[strlen(buf) - 1] == '\n')
                            buf[strlen(buf) - 1] = 0;
                        printf("Received from %d: %s\n", client_fd, buf);

                        if (login[client_fd] == 0) {
                            char user[32], pass[32], tmp[64];
                            int n = sscanf(buf, "%s%s%s", user, pass, tmp);
                            if (n != 2) {
                                char *msg = "Sai cu phap. Hay dang nhap lai.\n";
                                send(client_fd, msg, strlen(msg), 0);
                            } else {
                                sprintf(tmp, "%s %s", user, pass);
                                int found = 0;
                                char line[64];
                                FILE *f = fopen("users.txt", "r");
                                if (f != NULL) {
                                    while (fgets(line, sizeof(line), f) != NULL) {
                                        line[strcspn(line, "\r\n")] = 0;
                                        if (strcmp(line, tmp) == 0) {
                                            found = 1;
                                            break;
                                        }
                                    }
                                    fclose(f);
                                }

                                if (found == 1) {
                                    char *msg = "OK. Hay nhap lenh.\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                    login[client_fd] = 1;
                                } else {
                                    char *msg = "Sai username hoac password. Hay dang nhap lai.\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                }
                            }
                        } else {
                            char cmd[512];
                            sprintf(cmd, "%s > out.txt", buf);
                            system(cmd);
                            FILE *f = fopen("out.txt", "rb");
                            if (f != NULL) {
                                while (1) {
                                    int len = fread(buf, 1, sizeof(buf), f);
                                    if (len <= 0)
                                        break;
                                    send(client_fd, buf, len, 0);
                                }
                                fclose(f);
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_FDS 1024
#define MAX_TOPICS 10

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }
    
    listen(listener, 10);
    
    struct pollfd fds[MAX_FDS];
    char client_topics[MAX_FDS][MAX_TOPICS][64];
    int num_topics[MAX_FDS];

    for (int i = 0; i < MAX_FDS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        num_topics[i] = 0;
    }
    
    fds[0].fd = listener;
    char buf[1024];

    while (1) {
        int ret = poll(fds, MAX_FDS, -1);
        if (ret < 0) break;

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
                                num_topics[j] = 0; 
                                printf("New client connected: %d\n", client);
                                break;
                            }
                        }
                        if (j == MAX_FDS) close(client);
                    }
                } else {
                    int client_fd = fds[i].fd;
                    ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
                    
                    if (ret <= 0) {
                        printf("Client %d disconnected\n", client_fd);
                        close(client_fd);
                        fds[i].fd = -1;
                        num_topics[i] = 0; 
                    } else {
                        buf[ret] = 0;
                        buf[strcspn(buf, "\r\n")] = 0;
                        if (strlen(buf) == 0) continue;

                        char cmd[16] = {0}, topic[64] = {0}, msg[512] = {0};

                        int n = sscanf(buf, "%15s \"%63[^\"]\" %[^\n]", cmd, topic, msg);
                        
                        if (n < 2) {
                            n = sscanf(buf, "%15s %63s %[^\n]", cmd, topic, msg);
                        }

                        if (n >= 2 && strcmp(cmd, "SUB") == 0) {
                            int already_subbed = 0;
                            for (int t = 0; t < num_topics[i]; t++) {
                                if (strcmp(client_topics[i][t], topic) == 0) {
                                    already_subbed = 1;
                                    break;
                                }
                            }
                            
                            if (already_subbed) {
                                char *ok = "Already subscribed to this topic.\n";
                                send(client_fd, ok, strlen(ok), 0);
                            } else if (num_topics[i] < MAX_TOPICS) {
                                strcpy(client_topics[i][num_topics[i]], topic);
                                num_topics[i]++;
                                char *ok = "Subscribed successfully.\n";
                                send(client_fd, ok, strlen(ok), 0);
                            } else {
                                char *err = "Topic limit reached.\n";
                                send(client_fd, err, strlen(err), 0);
                            }
                        } 
                        else if (n >= 2 && strcmp(cmd, "UNSUB") == 0) {
                            int found = 0;
                            for (int t = 0; t < num_topics[i]; t++) {
                                if (strcmp(client_topics[i][t], topic) == 0) {
                                    strcpy(client_topics[i][t], client_topics[i][num_topics[i] - 1]);
                                    num_topics[i]--;
                                    found = 1;
                                    break;
                                }
                            }
                            
                            if (found) {
                                char *ok = "Unsubscribed successfully.\n";
                                send(client_fd, ok, strlen(ok), 0);
                            } else {
                                char *err = "Topic not found.\n";
                                send(client_fd, err, strlen(err), 0);
                            }
                        }
                        else if (n == 3 && strcmp(cmd, "PUB") == 0) {
                            char send_buf[1024];
                            snprintf(send_buf, sizeof(send_buf), "[%s]: %s\n", topic, msg);
                            
                            for (int k = 0; k < MAX_FDS; k++) {
                                if (fds[k].fd != -1 && fds[k].fd != listener) {
                                    for (int t = 0; t < num_topics[k]; t++) {
                                        if (strcmp(client_topics[k][t], topic) == 0) {
                                            send(fds[k].fd, send_buf, strlen(send_buf), 0);
                                            break;
                                        }
                                    }
                                }
                            }
                            char *ok = "Message published.\n";
                            send(client_fd, ok, strlen(ok), 0);
                        } else {
                            char *err = "Invalid syntax. Use: SUB <topic> | UNSUB <topic> | PUB <topic> <msg>\n";
                            send(client_fd, err, strlen(err), 0);
                        }
                    }
                }
            }
        }
    }
    
    close(listener);
    return 0;
}
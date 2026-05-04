#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;
    
    char f_user[64], f_pass[64];
    while (fscanf(f, "%63s %63s", f_user, f_pass) == 2) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

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
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }
    
    listen(listener, 10);
    printf("Telnet server is running on port %d...\n", port);
    
    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int states[FD_SETSIZE];
    memset(states, 0, sizeof(states));
    
    char temp_users[FD_SETSIZE][64];
    memset(temp_users, 0, sizeof(temp_users));

    char buf[1024];

    while (1) {
        fdtest = fdread;
        
        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);
        if (ret < 0) break;

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE) {
                        FD_SET(client, &fdread);
                        states[client] = 1; 
                        
                        char *msg = "Username: ";
                        send(client, msg, strlen(msg), 0);
                        printf("New connection: %d\n", client);
                    } else {
                        close(client);
                    }
                } else {
                    memset(buf, 0, sizeof(buf));
                    ret = recv(i, buf, sizeof(buf) - 1, 0);
                    
                    if (ret <= 0) {
                        printf("Client %d disconnected.\n", i);
                        close(i);
                        FD_CLR(i, &fdread);
                        states[i] = 0;
                    } else {
                        buf[strcspn(buf, "\r\n")] = 0;
                        if (strlen(buf) == 0) continue;

                        if (states[i] == 1) {
                            strcpy(temp_users[i], buf);
                            states[i] = 2;
                            
                            char *msg = "Password: ";
                            send(i, msg, strlen(msg), 0);
                        } 
                        else if (states[i] == 2) {
                            if (check_login(temp_users[i], buf)) {
                                states[i] = 3;
                                char *msg = "Login successful.\n> ";
                                send(i, msg, strlen(msg), 0);
                                printf("Client %d logged in as '%s'.\n", i, temp_users[i]);
                            } else {
                                char *msg = "Login failed. Disconnecting...\n";
                                send(i, msg, strlen(msg), 0);
                                printf("Client %d failed login.\n", i);
                                
                                close(i);
                                FD_CLR(i, &fdread);
                                states[i] = 0;
                            }
                        } 
                        else if (states[i] == 3) {
                            char cmd[1050];
                            snprintf(cmd, sizeof(cmd), "%s > out.txt 2>&1", buf);
                            system(cmd);

                            FILE *f = fopen("out.txt", "rb");
                            if (f) {
                                char file_buf[1024];
                                size_t bytes_read;
                                while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                                    send(i, file_buf, bytes_read, 0);
                                }
                                fclose(f);
                            } else {
                                char *err = "Error reading output.\n";
                                send(i, err, strlen(err), 0);
                            }
                            
                            char *prompt = "\n> ";
                            send(i, prompt, strlen(prompt), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}
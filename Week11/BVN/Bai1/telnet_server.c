#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

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
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }
    
    listen(listener, 10);
    printf("Multiprocess Telnet Server is listening on port 8080...\n");

    signal(SIGCHLD, signal_handler);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        printf("New client connected: %d\n", client);

        if (fork() == 0) { 
            close(listener); 
            char buf[1024];
            int logged_in = 0;

            char *prompt = "Vui long nhap 'user pass' de dang nhap:\n";
            send(client, prompt, strlen(prompt), 0);

            while (1) {
                int len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) break;
                
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0; 
                
                if (strlen(buf) == 0) continue;

                if (!logged_in) {
                    int found = 0;
                    FILE *f = fopen("users.txt", "r");
                    if (f != NULL) {
                        char line[256];
                        while (fgets(line, sizeof(line), f)) {
                            line[strcspn(line, "\r\n")] = 0;
                            if (strcmp(line, buf) == 0) {
                                found = 1;
                                break;
                            }
                        }
                        fclose(f);
                    }

                    if (found) {
                        logged_in = 1;
                        char *ok_msg = "Dang nhap thanh cong. Hay nhap lenh:\n";
                        send(client, ok_msg, strlen(ok_msg), 0);
                    } else {
                        char *err_msg = "Sai tai khoan. Hay nhap lai:\n";
                        send(client, err_msg, strlen(err_msg), 0);
                    }
                } else {
                    if (strcmp(buf, "exit") == 0) break;

                    char sys_cmd[2048];
                    char out_file[64];

                    sprintf(out_file, "out_%d.txt", getpid());
                    sprintf(sys_cmd, "%s > %s 2>&1", buf, out_file);
                    
                    system(sys_cmd);

                    FILE *f = fopen(out_file, "rb");
                    if (f != NULL) {
                        int read_len;
                        while ((read_len = fread(buf, 1, sizeof(buf), f)) > 0) {
                            send(client, buf, read_len, 0);
                        }
                        fclose(f);
                        remove(out_file);
                    }
                }
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
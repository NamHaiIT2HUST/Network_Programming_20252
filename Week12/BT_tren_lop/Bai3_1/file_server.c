#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>

#define DIR_PATH "./shared_folder"


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
    printf("File Server (Multiprocess) is listening on port 9000...\n");
    printf("Serving files from directory: %s\n", DIR_PATH);
    
    signal(SIGCHLD, signal_handler);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        printf("New client connected: %d\n", client);

        if (fork() == 0) {
            close(listener); 
            
            DIR *d = opendir(DIR_PATH);
            if (!d) {
                char *err = "ERROR No files to download \r\n";
                send(client, err, strlen(err), 0);
                close(client);
                exit(0);
            }

            struct dirent *dir;
            char file_list[4096] = "";
            int file_count = 0;

            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                    file_count++;
                    strcat(file_list, dir->d_name);
                    strcat(file_list, "\r\n");
                }
            }
            closedir(d);

            if (file_count == 0) {
                char *err = "ERROR No files to download \r\n";
                send(client, err, strlen(err), 0);
                close(client);
                exit(0);
            } else {
                char header[64];
                sprintf(header, "OK %d\r\n", file_count);
                send(client, header, strlen(header), 0);

                send(client, file_list, strlen(file_list), 0);
                send(client, "\r\n", 2, 0); 
            }

            char buf[256];
            while (1) {
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) break;
                
                buf[ret] = 0;
                buf[strcspn(buf, "\r\n")] = 0; 
                
                if (strlen(buf) == 0) continue;

                char filepath[512];
                snprintf(filepath, sizeof(filepath), "%s/%s", DIR_PATH, buf);

                FILE *f = fopen(filepath, "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long file_size = ftell(f);
                    fseek(f, 0, SEEK_SET); 

                    char size_header[64];
                    sprintf(size_header, "OK %ld\r\n", file_size);
                    send(client, size_header, strlen(size_header), 0);

                    char file_buf[1024];
                    int bytes_read;
                    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                        send(client, file_buf, bytes_read, 0);
                    }
                    fclose(f);

                    break; 
                } else {
                    char *err = "ERROR File khong ton tai. Vui long nhap lai ten file:\r\n";
                    send(client, err, strlen(err), 0);
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_BUF_SIZE 65535

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Syntax Error. Enter: %s <IP_Address> <Port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_ip);
    addr.sin_port = htons(server_port);

    printf("Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        close(client);
        return 1;
    }
    printf("Connected successfully!\n\n");

    char buffer[MAX_BUF_SIZE];
    int offset = 0;

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    uint16_t path_len = strlen(cwd);
    uint16_t net_path_len = htons(path_len); 
    memcpy(buffer + offset, &net_path_len, sizeof(net_path_len));
    offset += sizeof(net_path_len);

    memcpy(buffer + offset, cwd, path_len);
    offset += path_len;

    int num_files_offset = offset;
    uint16_t num_files = 0;
    offset += sizeof(uint16_t);

    DIR *dir = opendir(".");
    struct dirent *entry;
    struct stat file_stat;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (stat(entry->d_name, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                uint8_t name_len = strlen(entry->d_name);
                uint32_t file_size = htonl((uint32_t)file_stat.st_size);

                memcpy(buffer + offset, &name_len, sizeof(name_len));
                offset += sizeof(name_len);

                memcpy(buffer + offset, entry->d_name, name_len);
                offset += name_len;

                memcpy(buffer + offset, &file_size, sizeof(file_size));
                offset += sizeof(file_size);

                num_files++;
            }
        }
    }
    closedir(dir);

    uint16_t net_num_files = htons(num_files);
    memcpy(buffer + num_files_offset, &net_num_files, sizeof(net_num_files));

    int bytes_sent = send(client, buffer, offset, 0);
    if (bytes_sent > 0) {
        printf("=> Sent %d bytes of packed directory data to server.\n", bytes_sent);
    }

    close(client);
    return 0;
}
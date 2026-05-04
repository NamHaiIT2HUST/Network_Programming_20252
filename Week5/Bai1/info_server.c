#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUF_SIZE 65535

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Syntax Error. Enter: %s <Port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        return 1;
    }

    listen(listener, 5);

    while (1) {
        printf("[Server] Listening on port %d...\n", port);

        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;
        printf("[Server] A new client connected.\n");

        char buffer[MAX_BUF_SIZE];
        int n = recv(client, buffer, sizeof(buffer), 0);
        
        if (n <= 0) {
            printf("[Server] Client disconnected.\n\n");
            close(client);
            continue;
        }

        int offset = 0;

        uint16_t net_path_len;
        memcpy(&net_path_len, buffer + offset, sizeof(net_path_len));
        uint16_t path_len = ntohs(net_path_len);
        offset += sizeof(net_path_len);

        char cwd[1024] = {0};
        memcpy(cwd, buffer + offset, path_len);
        offset += path_len;
        
        printf("\n%s\n", cwd); 

        uint16_t net_num_files;
        memcpy(&net_num_files, buffer + offset, sizeof(net_num_files));
        uint16_t num_files = ntohs(net_num_files);
        offset += sizeof(net_num_files);

        for (int i = 0; i < num_files; i++) {
            uint8_t name_len;
            memcpy(&name_len, buffer + offset, sizeof(name_len));
            offset += sizeof(name_len);

            char filename[256] = {0};
            memcpy(filename, buffer + offset, name_len);
            offset += name_len;

            uint32_t net_file_size;
            memcpy(&net_file_size, buffer + offset, sizeof(net_file_size));
            uint32_t file_size = ntohl(net_file_size);
            offset += sizeof(net_file_size);

            printf("%s - %u bytes\n", filename, file_size);
        }
        
        printf("\n");
        close(client);
    }

    close(listener);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Syntax Error. Enter: %s <Port> <Greeting_File> <Output_File>\n", argv[0]);
        printf("Example: %s 9000 hello.txt log.txt\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *greeting_file = argv[2];
    char *output_file = argv[3];

    FILE *f_greet = fopen(greeting_file, "r");
    if (f_greet == NULL) {
        perror("Cannot open greeting file");
        return 1;
    }
    char greeting_msg[2048] = {0};
    fread(greeting_msg, 1, sizeof(greeting_msg) - 1, f_greet);
    fclose(f_greet);

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
    
    FILE *f_out = fopen(output_file, "a");
    if (f_out == NULL) {
        perror("Cannot open output file");
        return 1;
    }

    while (1) {
        printf("[Server] Listening on port %d...\n", port);
        
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            perror("accept() failed");
            continue;
        }
        printf("[Server] A new client connected.\n");

        send(client, greeting_msg, strlen(greeting_msg), 0);

        char buf[1024];
        while (1) {
            memset(buf, 0, sizeof(buf));
            int n = recv(client, buf, sizeof(buf) - 1, 0);
            
            if (n <= 0) {
                printf("[Server] Client disconnected.\n\n");
                break;
            }

            fprintf(f_out, "%s", buf);
            fflush(f_out);

            buf[strcspn(buf, "\n")] = 0; 
            printf("[Server] Saved to file: %s\n", buf);
        }
        close(client);
    }

    fclose(f_out);
    close(listener);
    return 0;
}
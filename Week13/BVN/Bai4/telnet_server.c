#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

// Luong (Thread) xu ly rieng cho tung Client
void *client_handler(void *arg) {
    int client = *(int *)arg;
    free(arg); // Giai phong bo nho cap phat dong

    char buf[1024];
    int logged_in = 0;

    char *prompt = "Vui long nhap 'user pass' de dang nhap:\n";
    send(client, prompt, strlen(prompt), 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0) break;
        
        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0; // Xoa Enter
        
        if (strlen(buf) == 0) continue;

        if (!logged_in) {
            // --- XU LY DANG NHAP ---
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
            // --- XU LY LENH TU CLIENT ---
            if (strcmp(buf, "exit") == 0) break;

            char sys_cmd[2048];
            char out_file[64];
            
            // Dung ma File Descriptor (client) lam ten file de chong trung lap giua cac Thread
            snprintf(out_file, sizeof(out_file), "out_fd_%d.txt", client);
            snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", buf, out_file);
            
            // Thuc thi lenh
            system(sys_cmd);

            // Doc file ket qua va gui tra lai Client
            FILE *f = fopen(out_file, "rb");
            if (f != NULL) {
                int read_len;
                while ((read_len = fread(buf, 1, sizeof(buf), f)) > 0) {
                    send(client, buf, read_len, 0);
                }
                fclose(f);
                remove(out_file); // Xoa file ngay sau khi gui de don dep he thong
            }
        }
    }
    
    printf("Client %d da ngat ket noi.\n", client);
    close(client);
    pthread_exit(NULL); // Ket thuc luong an toan
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);
    
    printf("Telnet Server (Multithreading) dang chay tren cong 8080...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        printf("Client moi ket noi: %d\n", client);

        // Cap phat dong chong Race Condition
        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, (void *)new_sock);
        pthread_detach(tid); // De he dieu hanh tu don RAM khi Thread xong viec
    }

    close(listener);
    return 0;
}
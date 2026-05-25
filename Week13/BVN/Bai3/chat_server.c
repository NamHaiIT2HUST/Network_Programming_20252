#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100

// Cau truc luu thong tin tung Client
typedef struct {
    int fd;
    char name[64];
} ClientNode;

ClientNode clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Ham gui tin nhan cho tat ca nguoi khac (Broadcast)
void broadcast_message(int sender_fd, char *sender_name, char *msg) {
    char out_buf[1024];
    char time_buf[64];
    
    // Lay thoi gian thuc he thong
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_buf, sizeof(time_buf), "%Y/%m/%d %I:%M:%S%p", tm_info);

    // Dinh dang: "2026/05/25 11:00:00PM name: message\n"
    snprintf(out_buf, sizeof(out_buf), "%s %s: %s\n", time_buf, sender_name, msg);

    // Khoa Mutex de an toan khi lap qua mang
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd != sender_fd) {
            send(clients[i].fd, out_buf, strlen(out_buf), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

// Luong (Thread) quan ly tung Client
void *client_handler(void *arg) {
    int client = *(int *)arg;
    free(arg); 

    char buf[512];
    char client_name[64];

    // Buoc 1: Yeu cau dang nhap dung cu phap
    char *prompt = "Vui long dang nhap theo cu phap: client_id: <ten_viet_lien>\n";
    send(client, prompt, strlen(prompt), 0);

    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            close(client);
            pthread_exit(NULL);
        }
        
        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0; // Xoa Enter

        // Kiem tra cu phap: client_id: ten
        if (sscanf(buf, "client_id: %63s", client_name) == 1) {
            char *ok_msg = "Dang nhap thanh cong! Ban co the bat dau chat.\n";
            send(client, ok_msg, strlen(ok_msg), 0);
            break; // Dang nhap dung -> Thoat vong lap login
        } else {
            char *err_msg = "Sai cu phap! Vui long nhap lai (VD: client_id: NamHai):\n";
            send(client, err_msg, strlen(err_msg), 0);
        }
    }

    // Buoc 2: Them vao danh sach Online
    pthread_mutex_lock(&lock);
    clients[client_count].fd = client;
    strcpy(clients[client_count].name, client_name);
    client_count++;
    pthread_mutex_unlock(&lock);

    printf("Client %s da tham gia phong chat.\n", client_name);

    // Buoc 3: Vong lap Chat
    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break; // Client thoat

        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;

        broadcast_message(client, client_name, buf);
    }

    // Buoc 4: Xy ly khi Client thoat
    printf("Client %s da thoat.\n", client_name);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == client) {
            // Xoa khoi mang bang cach de phan tu cuoi cung len
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    
    close(client);
    pthread_exit(NULL);
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
    
    printf("Chat Room Server (Multithreading) dang chay tren cong 8080...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        
        int *new_sock = malloc(sizeof(int));
        *new_sock = client;
        
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, (void *)new_sock);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}
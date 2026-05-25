#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define THREAD_POOL_SIZE 4  
#define QUEUE_SIZE 100      

// Cau truc Hang doi (Queue) chua Client
int client_queue[QUEUE_SIZE];
int q_count = 0, q_front = 0, q_rear = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 

// Luong cong nhan (Worker Thread) chay lien tuc
void *worker_thread(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);

    while (1) {
        int client;
        
        pthread_mutex_lock(&lock);
        while (q_count == 0) {
            pthread_cond_wait(&cond, &lock); // Cho den khi co khach
        }
        
        // Lay 1 Client khoi hang doi
        client = client_queue[q_front];
        q_front = (q_front + 1) % QUEUE_SIZE;
        q_count--;
        pthread_mutex_unlock(&lock);
        
        printf("[Thread %d] Dang phuc vu Client FD: %d\n", thread_id, client);

        // Xu ly HTTP Request
        char buf[2048];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = 0;
            puts(buf); // In Request cua Trinh duyet
            
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
            send(client, msg, strlen(msg), 0);
        }
        
        close(client);
    }
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
    
    printf("HTTP Server (Prethreading) dang lang nghe tren cong 8080...\n");

    // Khởi tạo Thread Pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_t tid;
        pthread_create(&tid, NULL, worker_thread, id);
        pthread_detach(tid);
    }

    // Luong chinh (Lễ tân) chỉ làm nhiệm vụ nhận kết nối
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        pthread_mutex_lock(&lock);
        if (q_count < QUEUE_SIZE) {
            client_queue[q_rear] = client;
            q_rear = (q_rear + 1) % QUEUE_SIZE;
            q_count++;
            
            pthread_cond_signal(&cond); // Đánh thức 1 luồng để phục vụ
        } else {
            close(client); // Hàng đợi đầy
        }
        pthread_mutex_unlock(&lock);
    }

    close(listener);
    return 0;
}
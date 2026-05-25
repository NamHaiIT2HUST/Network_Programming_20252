#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

// Khóa Mutex bảo vệ biến hàng đợi tránh xung đột luồng
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int waiting_client = -1; // -1 nghĩa là hàng đợi đang trống

// Hàm chạy trong Thread riêng biệt để phục vụ từng Cặp (Pair)
void *pair_handler(void *arg) {
    int *fds = (int *)arg;
    int c1 = fds[0];
    int c2 = fds[1];
    free(arg); // Giải phóng vùng nhớ do main cấp phát

    struct pollfd pfds[2];
    pfds[0].fd = c1; pfds[0].events = POLLIN;
    pfds[1].fd = c2; pfds[1].events = POLLIN;

    char buf[1024];
    
    while (1) {
        int ret = poll(pfds, 2, -1);
        if (ret < 0) break;

        // Nếu Client 1 gửi tin
        if (pfds[0].revents & POLLIN) {
            int n = recv(c1, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("Client %d disconnected.\n", c1);
                break; // Thoát vòng lặp để đóng kết nối
            }
            send(c2, buf, n, 0); // Chuyển tiếp sang Client 2
        }

        // Nếu Client 2 gửi tin
        if (pfds[1].revents & POLLIN) {
            int n = recv(c2, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("Client %d disconnected.\n", c2);
                break; // Thoát vòng lặp để đóng kết nối
            }
            send(c1, buf, n, 0); // Chuyển tiếp sang Client 1
        }
    }

    // Xử lý yêu cầu: 1 người thoát thì ngắt luôn người còn lại
    printf("Closing chat room for pair (%d, %d)\n", c1, c2);
    char *msg = "\n[System] Partner disconnected. Connection closed.\n";
    send(c1, msg, strlen(msg), 0);
    send(c2, msg, strlen(msg), 0);
    
    close(c1);
    close(c2);
    
    pthread_exit(NULL); // Kết thúc luồng
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) return 1;

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) return 1;
    listen(listener, 10);
    
    printf("Multithreaded Pairing Chat Server listening on port 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        
        printf("New client connected: %d\n", client);

        // Khóa Mutex trước khi đụng vào hàng đợi chung
        pthread_mutex_lock(&lock);
        
        if (waiting_client == -1) {
            // Trường hợp 1: Hàng đợi rỗng -> Cho người này vào chờ
            waiting_client = client;
            char *msg = "[System] Waiting for a partner to join...\n";
            send(client, msg, strlen(msg), 0);
            pthread_mutex_unlock(&lock); // Xong việc phải mở khóa ngay
        } else {
            // Trường hợp 2: Đã có người chờ -> Ghép cặp
            int c1 = waiting_client;
            int c2 = client;
            waiting_client = -1; // Reset hàng đợi để đón cặp tiếp theo
            pthread_mutex_unlock(&lock);

            char *msg = "[System] Partner found! You can start chatting.\n";
            send(c1, msg, strlen(msg), 0);
            send(c2, msg, strlen(msg), 0);

            // Cấp phát mảng chứa 2 file descriptor để đẩy vào Thread
            int *fds = malloc(2 * sizeof(int));
            fds[0] = c1;
            fds[1] = c2;
            
            // Khởi tạo luồng (Thread) mới để phục vụ cặp này
            pthread_t tid;
            pthread_create(&tid, NULL, pair_handler, fds);
            pthread_detach(tid); // Tự động dọn dẹp RAM khi luồng kết thúc (tránh Zombie Thread)
        }
    }

    close(listener);
    return 0;
}
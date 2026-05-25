#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

// Mutex bao ve hang doi tranh xung dot luong
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int waiting_client = -1; 

// Luong (Thread) xu ly rieng cho 1 cap
void *pair_handler(void *arg) {
    int *fds = (int *)arg;
    int c1 = fds[0];
    int c2 = fds[1];
    free(arg); 

    // Dung poll de lang nghe ca 2 client cung luc
    struct pollfd pfds[2];
    pfds[0].fd = c1; pfds[0].events = POLLIN;
    pfds[1].fd = c2; pfds[1].events = POLLIN;

    char buf[1024];
    
    while (1) {
        int ret = poll(pfds, 2, -1);
        if (ret < 0) break;

        // Client 1 gui -> Chuyen tiep Client 2
        if (pfds[0].revents & POLLIN) {
            int n = recv(c1, buf, sizeof(buf) - 1, 0);
            if (n <= 0) break; 
            send(c2, buf, n, 0); 
        }

        // Client 2 gui -> Chuyen tiep Client 1
        if (pfds[1].revents & POLLIN) {
            int n = recv(c2, buf, sizeof(buf) - 1, 0);
            if (n <= 0) break; 
            send(c1, buf, n, 0); 
        }
    }

    // 1 trong 2 ngat ket noi -> Dong phong chat
    char *msg = "\n[He thong] Doi tac da thoat. Ket thuc chat.\n";
    send(c1, msg, strlen(msg), 0);
    send(c2, msg, strlen(msg), 0);
    
    close(c1);
    close(c2);
    printf("Da dong phong chat cua cap (%d, %d)\n", c1, c2);
    
    pthread_exit(NULL);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);
    
    printf("Server Chat Ghep Cap dang chay tren cong 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        
        printf("Client moi ket noi: %d\n", client);

        pthread_mutex_lock(&lock);
        
        if (waiting_client == -1) {
            // Hang doi rong -> Cho vao cho
            waiting_client = client;
            char *msg = "[He thong] Dang tim kiem doi tac...\n";
            send(client, msg, strlen(msg), 0);
            pthread_mutex_unlock(&lock);
        } else {
            // Da co nguoi cho -> Ghep cap
            int c1 = waiting_client;
            int c2 = client;
            waiting_client = -1; 
            pthread_mutex_unlock(&lock);

            char *msg = "[He thong] Ghep cap thanh cong! Ban co the bat dau chat.\n";
            send(c1, msg, strlen(msg), 0);
            send(c2, msg, strlen(msg), 0);

            // Cap phat dong de truyen vao Thread
            int *fds = malloc(2 * sizeof(int));
            fds[0] = c1;
            fds[1] = c2;
            
            pthread_t tid;
            pthread_create(&tid, NULL, pair_handler, fds);
            pthread_detach(tid); 
        }
    }

    close(listener);
    return 0;
}
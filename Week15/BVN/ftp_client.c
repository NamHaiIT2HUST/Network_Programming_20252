#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define MAX_BUF 4096

void receive_response(int sock) {
    char buf[MAX_BUF];
    int len = recv(sock, buf, sizeof(buf) - 1, 0);
    if (len > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }
}

void send_command(int sock, const char *cmd) {
    printf("> %s", cmd);
    send(sock, cmd, strlen(cmd), 0);
}

int enter_pasv(int control_sock) {
    char buf[MAX_BUF];
    send_command(control_sock, "PASV\r\n");
    int len = recv(control_sock, buf, sizeof(buf) - 1, 0);
    if (len > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }

    int h1, h2, h3, h4, p1, p2;
    char *paren = strchr(buf, '(');
    if (!paren) return -1;
    sscanf(paren, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);

    char ip[32];
    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    int port = p1 * 256 + p2;

    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in data_addr = {0};
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    data_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("Data connection failed");
        return -1;
    }
    return data_sock;
}

int main() {
    int control_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct addrinfo *res;
    getaddrinfo("lebavui.io.vn", "21", NULL, &res);
    connect(control_sock, res->ai_addr, res->ai_addrlen);
    receive_response(control_sock); 

    send_command(control_sock, "USER user_20235321\r\n");
    receive_response(control_sock);
    
    send_command(control_sock, "PASS 532123\r\n");
    receive_response(control_sock);

    int data_sock = enter_pasv(control_sock);
    send_command(control_sock, "NLST\r\n");
    receive_response(control_sock);

    char file_list[MAX_BUF] = {0};
    int bytes, total = 0;
    while ((bytes = recv(data_sock, file_list + total, sizeof(file_list) - total - 1, 0)) > 0) {
        total += bytes;
    }
    close(data_sock);
    receive_response(control_sock);

    char question_file[256] = "";
    char answer_file[256] = "";
    char *line = strtok(file_list, "\r\n");
    while (line != NULL) {
        if (strncmp(line, "question_", 9) == 0) {
            strcpy(question_file, line);
            break;
        }
        line = strtok(NULL, "\r\n");
    }

    if (strlen(question_file) == 0) {
        printf("[-] Khong tim thay file question tren server!\n");
        return 1;
    }
    printf("[+] Tim thay file: %s\n", question_file);

    strcpy(answer_file, question_file);
    strncpy(answer_file, "answer", 6);

    data_sock = enter_pasv(control_sock);
    char retr_cmd[512];
    sprintf(retr_cmd, "RETR %s\r\n", question_file);
    send_command(control_sock, retr_cmd);
    receive_response(control_sock);

    char content[MAX_BUF] = {0};
    total = 0;
    while ((bytes = recv(data_sock, content + total, sizeof(content) - total - 1, 0)) > 0) {
        total += bytes;
    }
    close(data_sock);
    receive_response(control_sock);

    printf("[+] Noi dung question (100 ky tu):\n%s\n", content);

    char reversed_content[MAX_BUF] = {0};
    for (int i = 0; i < total; i++) {
        reversed_content[i] = content[total - 1 - i];
    }
    printf("[+] Noi dung answer:\n%s\n", reversed_content);

    FILE *f = fopen(answer_file, "wb");
    if (f) {
        fwrite(reversed_content, 1, total, f);
        fclose(f);
        printf("[+] Da tao file local: %s\n", answer_file);
    }

    data_sock = enter_pasv(control_sock);
    char stor_cmd[512];
    sprintf(stor_cmd, "STOR %s\r\n", answer_file);
    send_command(control_sock, stor_cmd);
    receive_response(control_sock);

    send(data_sock, reversed_content, total, 0); 
    close(data_sock);
    receive_response(control_sock);

    printf("[+] Upload thanh cong file: %s\n", answer_file);

    send_command(control_sock, "QUIT\r\n");
    receive_response(control_sock);

    close(control_sock);
    freeaddrinfo(res);
    return 0;
}
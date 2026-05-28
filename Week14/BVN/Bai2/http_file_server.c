#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>

void url_decode(char *src, char *dest) {
    char *p = src;
    char code[3] = {0};
    while (*p) {
        if (*p == '%') {
            memcpy(code, p + 1, 2);
            *dest++ = (char)strtol(code, NULL, 16);
            p += 3;
        } else if (*p == '+') {
            *dest++ = ' ';
            p++;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".c") == 0) return "text/plain";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    return "application/octet-stream";
}

void *client_handler(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char buf[4096];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret <= 0) {
        close(client);
        pthread_exit(NULL);
    }
    buf[ret] = 0;

    char method[16], raw_path[2048], req_path[2048];
    sscanf(buf, "%s %s", method, raw_path);

    if (strcmp(method, "GET") != 0) {
        close(client);
        pthread_exit(NULL);
    }

    url_decode(raw_path, req_path);

    if (strlen(req_path) > 1 && req_path[strlen(req_path) - 1] == '/') {
        req_path[strlen(req_path) - 1] = '\0';
    }

    char real_path[4096];
    sprintf(real_path, ".%s", req_path); 

    struct stat path_stat;
    if (stat(real_path, &path_stat) == -1) {
        char *err_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 File Not Found</h1>";
        send(client, err_msg, strlen(err_msg), 0);
        close(client);
        pthread_exit(NULL);
    }

    if (S_ISDIR(path_stat.st_mode)) {
        char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
        send(client, header, strlen(header), 0);

        char html[8192];
        sprintf(html, "<html><head><title>Index of %s</title></head><body>", req_path);
        sprintf(html + strlen(html), "<h2>Danh muc: %s</h2><hr><ul>", req_path);
        send(client, html, strlen(html), 0);

        if (strcmp(req_path, "/") != 0) {
            char back_link[] = "<li><b><a href=\"..\">.. (Quay lai)</a></b></li>";
            send(client, back_link, strlen(back_link), 0);
        }

        DIR *dir = opendir(real_path);
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

            char item_html[8192];
            char link_path[4096];

            if (strcmp(req_path, "/") == 0) sprintf(link_path, "/%s", ent->d_name);
            else sprintf(link_path, "%s/%s", req_path, ent->d_name);

            if (ent->d_type == DT_DIR) {
                sprintf(item_html, "<li><b><a href=\"%s\">%s/</a></b></li>", link_path, ent->d_name);
            } else {
                sprintf(item_html, "<li><i><a href=\"%s\">%s</a></i></li>", link_path, ent->d_name);
            }
            send(client, item_html, strlen(item_html), 0);
        }
        closedir(dir);

        char footer[] = "</ul><hr></body></html>";
        send(client, footer, strlen(footer), 0);
    } 
    else if (S_ISREG(path_stat.st_mode)) {
        FILE *f = fopen(real_path, "rb");
        if (f) {
            char header[1024];
            sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", 
                    get_mime_type(real_path), path_stat.st_size);
            send(client, header, strlen(header), 0);

            char file_buf[8192];
            int bytes_read;
            while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                send(client, file_buf, bytes_read, 0);
            }
            fclose(f);
        }
    }

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

    printf("HTTP File Server dang chay tren cong 8080...\n");
    printf("Mo trinh duyet truy cap: http://127.0.0.1:8080\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;

        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, new_sock);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}
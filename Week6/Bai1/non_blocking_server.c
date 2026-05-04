#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>

void generate_email(char *hoten, char *mssv, char *email) {
    char *words[20];
    int num_words = 0;
    char temp[256];
    strcpy(temp, hoten);

    char *token = strtok(temp, " ");
    while (token != NULL) {
        words[num_words++] = token;
        token = strtok(NULL, " ");
    }

    if (num_words == 0) return;

    char ten[64] = {0};
    strcpy(ten, words[num_words - 1]);
    for (int i = 0; ten[i]; i++) ten[i] = tolower(ten[i]);

    char initials[20] = {0};
    for (int i = 0; i < num_words - 1; i++) {
        initials[i] = tolower(words[i][0]);
    }

    char mssv_suffix[20] = {0};
    if (strlen(mssv) >= 6) {
        strcpy(mssv_suffix, mssv + 2);
    } else {
        strcpy(mssv_suffix, mssv);
    }

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", ten, initials, mssv_suffix);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    int clients[64];
    int states[64];           
    char names[64][256];      
    int nclients = 0;

    char buf[256];
    int len;

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New client accepted!\n");
            clients[nclients] = client;
            
            states[nclients] = 0; 
            memset(names[nclients], 0, sizeof(names[nclients]));
            nclients++;
            
            ul = 1;
            ioctl(client, FIONBIO, &ul);

            char *msg = "Ho ten: ";
            send(client, msg, strlen(msg), 0);
        }

        for (int i = 0; i < nclients; i++) {
            len = recv(clients[i], buf, sizeof(buf) - 1, 0);
            if (len == -1) {
                if (errno != EWOULDBLOCK) {
                    continue;
                }
            } else if (len == 0) {
                printf("Client disconnected.\n");
                close(clients[i]);
                clients[i] = clients[nclients - 1];
                states[i] = states[nclients - 1];
                strcpy(names[i], names[nclients - 1]);
                nclients--;
                i--;
            } else {
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0; 
                if (strlen(buf) == 0) continue;

                if (states[i] == 0) {
                    strcpy(names[i], buf); 
                    states[i] = 1;         
                    
                    char *msg = "MSSV: ";
                    send(clients[i], msg, strlen(msg), 0);
                } 
                else if (states[i] == 1) {
                    char email[256];
                    
                    generate_email(names[i], buf, email);
                    
                    char response[512];
                    sprintf(response, "=> Email HUST: %s\n\n", email);
                    send(clients[i], response, strlen(response), 0);
                    
                    printf("Da tao email\n");
                    close(clients[i]);
                    
                    clients[i] = clients[nclients - 1];
                    states[i] = states[nclients - 1];
                    strcpy(names[i], names[nclients - 1]);
                    nclients--;
                    i--;
                }
            }
        }
        usleep(10000); 
    }

    close(listener);
    return 0;
}
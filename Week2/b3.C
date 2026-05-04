#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char ftp_response[256];
    fgets(ftp_response, sizeof(ftp_response), stdin);

    int h1, h2, h3, h4, p1, p2;  

    char *start = strchr(ftp_response, '(');

    if (start != NULL) {
        if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
            int port = p1 * 256 + p2;

            printf("--- KET QUA PHAN TICH PASV ---\n");
            printf("Dia chi IP : %d.%d.%d.%d\n", h1, h2, h3, h4);
            printf("Cong (Port): %d\n", port);
            
        } else {
            printf("Loi: Khong the trich xuat cac tham so PASV.\n");
        }
        
    } else {
        printf("Loi: Chuoi phan hoi FTP khong dung dinh dang.\n");
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool check(char cmd[], int n) {
    if (n != 3) {
        return false;
    }

    if (strcmp(cmd, "ADD") == 0 || strcmp(cmd, "SUB") == 0 || 
        strcmp(cmd, "MUL") == 0 || strcmp(cmd, "DIV") == 0) {
        return true;
    }

    return false;
}

int main() {
    char str[32];
    while (1) {
        printf("Nhap lenh: ");
        fgets(str, sizeof(str), stdin);

        char cmd[10]; 
        float p1, p2;
    
        int n = sscanf(str, "%s %f %f", cmd, &p1, &p2);

        if (check(cmd, n)) {
            printf("Cau lenh dung cu phap\n");
            printf("cmd = %s\n", cmd);
            printf("p1 = %.2f\n", p1); 
            printf("p2 = %.2f\n", p2);
        }
        else {
            printf("Cau lenh sai cu phap\n");
        }    
    }

    return 0;
}
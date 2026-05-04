#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    unsigned char header[] = {
        0x45, 0x20, 0x00, 0x28, 0x9b, 0xa4, 0x00, 0x00, 0x6f, 0x06,
        0x2f, 0x38, 0x28, 0x7e, 0x23, 0x54, 0xac, 0x15, 0x88, 0xec
    };

    int version = header[0] >> 4;
    printf("Version = %d\n", version);

    int ihl = header[0] & 0x0F;
    printf("IHL = %d bytes\n", version * 4);

    int total_length = header[2] * 256 + header[3];
    printf("Total length = %d bytes\n", total_length);
    printf("Source IP: %d.%d.%d.%d\n", header[12], header[13], header[14], header[15]);
    printf("Destionaion IP: %d.%d.%d.%d\n", header[16], header[17], header[18], header[19]);
}
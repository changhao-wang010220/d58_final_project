#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
 
#define BUF_SIZE (8192)
#define PORT 10001
 
unsigned char fileBuf[BUF_SIZE];
 
void file_client(const char *ip, const char *path) {
    int client_socket;
    FILE *fp = NULL;
    struct sockaddr_in sockAddr;
    unsigned int fileSize, fileSize2;
    int size, nodeSize;
 
    if((client_socket=socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket");
        exit(1);
    } else {
        printf("socket success!\n");
    }
 
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(PORT);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &sockAddr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
 
    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
 
    size = read(client_socket, (unsigned char *)&fileSize, 4);
    if( size != 4 ) {
        printf("file size error!\n");
        close(client_socket);
        exit(-1);
    }
    printf("file size:%d\n", fileSize);
 
    if( (size = write(client_socket, "OK", 2) ) < 0 ) {
        perror("write");
        close(client_socket);
        exit(1);
    }
 
    fp = fopen(path, "w");
    if( fp == NULL ) {
        perror("fopen");
        close(client_socket);
        return;
    }
 
    fileSize2 = 0;
    while(memset(fileBuf, 0, sizeof(fileBuf)), (size = read(client_socket, fileBuf, sizeof(fileBuf))) > 0) {
        unsigned int size2 = 0;
        while( size2 < size ) {
            if( (nodeSize = fwrite(fileBuf + size2, 1, size - size2, fp) ) < 0 ) {
                perror("write");
                close(client_socket);
                exit(1);
            }
            size2 += nodeSize;
        }
        fileSize2 += size;
        if(fileSize2 >= fileSize) {
            break;
        }
    }
    fclose(fp);
    close(client_socket);
}
 
int main(int argc, char **argv) {
    if( argc < 3 ) {
        printf("file client: argument error!\n");
        return -1;
    }
 
    file_client(argv[1], argv[2]);
 
    return 0;
}
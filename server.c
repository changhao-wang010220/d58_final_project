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
#include <pthread.h>

#define BUF_SIZE (8192)
#define PORT 10001

unsigned char fileBuf[BUF_SIZE];

/*
 * Function to set up the socket
 */
int setup_socket() {
    int skfd;
    struct sockaddr_in sockAddr;

    if ((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    } else {
        printf("socket success!\n");
    }

    memset(&sockAddr, 0, sizeof(struct sockaddr_in));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(PORT);

    // bind
    if (bind(skfd, (struct sockaddr *)(&sockAddr), sizeof(struct sockaddr)) < 0) {
        perror("Bind");
        exit(1);
    } else {
        printf("bind success!\n");
    }

    if (listen(skfd, 4) < 0) {
        perror("Listen");
        exit(1);
    } else {
        printf("Server listening on port %d...\n", PORT);
    }

    return skfd;
}

/*
 * Function to receive file path from the client
 */
char *receive_file_path(int cnfd) {
    static char path[256];  // Assuming a maximum file path length of 255 characters

    if (read(cnfd, path, sizeof(path)) < 0) {
        perror("read");
        exit(1);
    }

    return path;
}

/*
 * send file
 */
void *handle_client(void *arg) {
    int cnfd = *((int *)arg);
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    // Receive file path from the client
    char *path = receive_file_path(cnfd);
    printf("Received file path: %s\n", path);

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        close(cnfd);
        pthread_exit(NULL);
    }

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (write(cnfd, (unsigned char *)&fileSize, 4) != 4) {
        perror("write");
        close(cnfd);
        fclose(fp);
        pthread_exit(NULL);
    }

    if (read(cnfd, buf, 2) != 2) {
        perror("read");
        close(cnfd);
        fclose(fp);
        pthread_exit(NULL);
    }

    while ((size = fread(fileBuf, 1, BUF_SIZE, fp)) > 0) {
        unsigned int size2 = 0;
        while (size2 < size) {
            if ((netSize = write(cnfd, fileBuf + size2, size - size2)) < 0) {
                perror("write");
                close(cnfd);
                fclose(fp);
                pthread_exit(NULL);
            }
            size2 += netSize;
        }
    }

    fclose(fp);
    close(cnfd);
    pthread_exit(NULL);
}

int main() {
    int skfd = setup_socket();

    while (1) {
        int cnfd;
        struct sockaddr_in cltAddr;
        socklen_t addrLen = sizeof(struct sockaddr_in);

        // Accept a new client connection
        if ((cnfd = accept(skfd, (struct sockaddr *)(&cltAddr), &addrLen)) < 0) {
            perror("Accept");
            continue;
        } else {
            printf("accept success!\n");
        }

        // Create a new thread to handle the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, &cnfd) != 0) {
            perror("pthread_create");
            close(cnfd);
            continue;
        }

        // Detach the thread so it can automatically clean up resources when it finishes
        pthread_detach(thread);
    }

    close(skfd);

    return 0;
}

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
#include <sys/wait.h>

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
 * Function to receive file paths from the client
 */
void receive_file_paths(int cnfd, char *sourcePath, size_t sourcePathSize, char *storePath, size_t storePathSize) {
    // Receive source path from the client
    if (read(cnfd, sourcePath, sourcePathSize) < 0) {
        perror("read");
        exit(1);
    }

    // Receive store path from the client
    if (read(cnfd, storePath, storePathSize) < 0) {
        perror("read");
        exit(1);
    }
}

/*
 * Function to handle a client connection
 */
void handle_client(int cnfd) {
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    // Receive file paths from the client
    char sourcePath[256];
    char storePath[256];
    receive_file_paths(cnfd, sourcePath, sizeof(sourcePath), storePath, sizeof(storePath));
    printf("Received source path: %s\n", sourcePath);
    printf("Received store path: %s\n", storePath);

    fp = fopen(sourcePath, "r");
    if (fp == NULL) {
        perror("fopen");
        close(cnfd);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (write(cnfd, (unsigned char *)&fileSize, 4) != 4) {
        perror("write");
        close(cnfd);
        fclose(fp);
        exit(1);
    }

    if (read(cnfd, buf, 2) != 2) {
        perror("read");
        close(cnfd);
        fclose(fp);
        exit(1);
    }

    while ((size = fread(fileBuf, 1, BUF_SIZE, fp)) > 0) {
        unsigned int size2 = 0;
        while (size2 < size) {
            if ((netSize = write(cnfd, fileBuf + size2, size - size2)) < 0) {
                perror("write");
                close(cnfd);
                fclose(fp);
                exit(1);
            }
            size2 += netSize;
        }
    }

    fclose(fp);
    close(cnfd);
    exit(0);
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

        // Create a new process to handle the client
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(cnfd);
            continue;
        } else if (pid == 0) { // Child process
            close(skfd); // Close the listener in the child process
            handle_client(cnfd);
        } else { // Parent process
            close(cnfd); // Close the connection in the parent process
            // Optionally, you can add code here to handle the parent process logic
        }
    }

    close(skfd);

    return 0;
}

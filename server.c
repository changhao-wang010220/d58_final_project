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
void receive_file_paths(int cnfd, char *sourcePath, size_t sourcePathSize, char *storePath, size_t storePathSize, char *mode, size_t modeSize) {
    // Receive source path length from the client
    size_t sourcePathLen;
    if (read(cnfd, &sourcePathLen, sizeof(size_t)) != sizeof(size_t)) {
        perror("read");
        exit(1);
    }

    // Receive source path from the client
    if (read(cnfd, sourcePath, sourcePathLen) != sourcePathLen) {
        perror("read");
        exit(1);
    }

    // Receive store path length from the client
    size_t storePathLen;
    if (read(cnfd, &storePathLen, sizeof(size_t)) != sizeof(size_t)) {
        perror("read");
        exit(1);
    }

    // Receive store path from the client
    if (read(cnfd, storePath, storePathLen) != storePathLen) {
        perror("read");
        exit(1);
    }
    char str[1024] = "./server_files/";
    strcat(str, storePath);
    strcpy(storePath, str);
    // Receive mode length from the client
    size_t modeLen;
    if (read(cnfd, &modeLen, sizeof(size_t)) != sizeof(size_t)) {
        perror("read");
        exit(1);
    }
    // Receive mode from the client
    if (read(cnfd, mode, modeLen) != modeLen) {
        perror("read");
        exit(1);
    }
}

void receive_file(int cnfd, const char *storePath) {
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    size = read(cnfd, (unsigned char *)&fileSize, sizeof(fileSize));
    if (size != sizeof(fileSize)) {
        printf("file size error!\n");
        return;
    }
    printf("file size:%d\n", fileSize);

    if ((size = write(cnfd, "OK", 2)) < 0) {
        perror("write");
        return;
    }

    fp = fopen(storePath, "w");
    if (fp == NULL) {
        perror("fopen");
        return;
    }

    unsigned int fileSize2 = 0;
    while (fileSize2 < fileSize) {
        memset(fileBuf, 0, sizeof(fileBuf));
        size = read(cnfd, fileBuf, sizeof(fileBuf));
        if (size <= 0) {
            perror("read");
            fclose(fp);
            close(cnfd);
            exit(1);
        }

        unsigned int size2 = 0;
        while (size2 < size) {
            if ((netSize = fwrite(fileBuf + size2, 1, size - size2, fp)) < 0) {
                perror("write");
                fclose(fp);
                close(cnfd);
                exit(1);
            }
            size2 += netSize;
        }
        fileSize2 += size;
    }
    
    fclose(fp);
}

/*
 * Function to send the file to the client
 */
void send_file_to_client(int cnfd, const char *sourcePath) {
    FILE *fp = fopen(sourcePath, "r");

    if (fp == NULL) {
        perror("fopen");
        close(cnfd);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    unsigned int fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (write(cnfd, (unsigned char *)&fileSize, sizeof(fileSize)) != sizeof(fileSize)) {
        perror("write");
        close(cnfd);
        fclose(fp);
        exit(1);
    }

    size_t bytesRead;
    while ((bytesRead = fread(fileBuf, 1, sizeof(fileBuf), fp)) > 0) {
        if (write(cnfd, fileBuf, bytesRead) < 0) {
            perror("write");
            close(cnfd);
            fclose(fp);
            exit(1);
        }
    }

    fclose(fp);
}

/*
 * Function to handle client connections
 */
void handle_client(int cnfd) {
    // Receive file paths from the client
    char sourcePath[256];
    char storePath[256];
    char modeString[256];
    receive_file_paths(cnfd, sourcePath, sizeof(sourcePath), storePath, sizeof(storePath), modeString, sizeof(modeString));
    printf("Received source path: %s\n", sourcePath);
    printf("Received store path: %s\n", storePath);
    printf("Received mode      : %s\n", modeString);

    // Send the file to the client
    if (strcmp(modeString, "r") == 0)
    {
        send_file_to_client(cnfd, sourcePath);
    }
    else if (strcmp(modeString, "s") == 0){
        receive_file(cnfd, storePath);
    }

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

        if (pid == 0) {
            // Child process
            close(skfd);  // Close the server socket in the child process
            handle_client(cnfd);
        } else if (pid < 0) {
            perror("fork");
            close(cnfd);
        } else {
            // Parent process
            close(cnfd);  // Close the client socket in the parent process
        }
    }

    close(skfd);

    return 0;
}

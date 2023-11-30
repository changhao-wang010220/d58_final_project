#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUF_SIZE (8192)
#define PORT 10001

unsigned char fileBuf[BUF_SIZE];
int client_socket;

// Function to handle Ctrl+C
void handle_ctrl_c(int signal) {
    printf("\nCtrl+C pressed. Closing the client.\n");
    close(client_socket);
    exit(0);
}

/*
 * Function to send file paths to the server
 */
void send_file_paths(const char *sourcePath, const char *storePath) {
    // Send source path to the server
    if (write(client_socket, sourcePath, strlen(sourcePath) + 1) < 0) {
        perror("write");
        close(client_socket);
        exit(1);
    }

    // Send store path to the server
    if (write(client_socket, storePath, strlen(storePath) + 1) < 0) {
        perror("write");
        close(client_socket);
        exit(1);
    }
}

/*
 * Function to receive the file from the server
 */
void file_client(const char *ip) {
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    } else {
        printf("socket success!\n");
    }

    struct sockaddr_in sockAddr;
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

    // Set up a signal handler for Ctrl+C
    signal(SIGINT, handle_ctrl_c);

    while (1) {
        // Read source path from standard input
        char sourcePath[256];
        printf("Enter source path (Ctrl+C to exit): ");
        if (fgets(sourcePath, sizeof(sourcePath), stdin) == NULL) {
            break;  // Break if EOF (Ctrl+C)
        }
        sourcePath[strcspn(sourcePath, "\n")] = '\0';  // Remove newline character

        // Read store path from standard input
        char storePath[256];
        printf("Enter store path: ");
        if (fgets(storePath, sizeof(storePath), stdin) == NULL) {
            break;  // Break if EOF (Ctrl+C)
        }
        storePath[strcspn(storePath, "\n")] = '\0';  // Remove newline character

        // Send file paths to the server
        send_file_paths(sourcePath, storePath);

        size = read(client_socket, (unsigned char *)&fileSize, 4);
        if (size != 4) {
            printf("file size error!\n");
            break;
        }
        printf("file size:%d\n", fileSize);

        if ((size = write(client_socket, "OK", 2)) < 0) {
            perror("write");
            break;
        }

        fp = fopen(storePath, "w");
        if (fp == NULL) {
            perror("fopen");
            break;
        }

        unsigned int fileSize2 = 0;
        while (memset(fileBuf, 0, sizeof(fileBuf)), (size = read(client_socket, fileBuf, sizeof(fileBuf))) > 0) {
            unsigned int size2 = 0;
            while (size2 < size) {
                if ((netSize = fwrite(fileBuf + size2, 1, size - size2, fp)) < 0) {
                    perror("write");
                    fclose(fp);
                    close(client_socket);
                    exit(1);
                }
                size2 += netSize;
            }
            fileSize2 += size;
            if (fileSize2 >= fileSize) {
                break;
            }
        }

        fclose(fp);
    }

    close(client_socket);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("file client: argument error!\n");
        return -1;
    }

    file_client(argv[1]);

    return 0;
}

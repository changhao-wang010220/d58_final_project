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
/*
 * Function to send file paths to the server
 */
void send_file_paths(const char *sourcePath, const char *storePath) {
    // Send source path to the server
    size_t sourcePathLen = strlen(sourcePath) + 1;
    if (write(client_socket, &sourcePathLen, sizeof(size_t)) != sizeof(size_t)) {
        perror("write");
        close(client_socket);
        exit(1);
    }
    if (write(client_socket, sourcePath, sourcePathLen) != sourcePathLen) {
        perror("write");
        close(client_socket);
        exit(1);
    }

    // Send store path to the server
    size_t storePathLen = strlen(storePath) + 1;
    if (write(client_socket, &storePathLen, sizeof(size_t)) != sizeof(size_t)) {
        perror("write");
        close(client_socket);
        exit(1);
    }
    if (write(client_socket, storePath, storePathLen) != storePathLen) {
        perror("write");
        close(client_socket);
        exit(1);
    }
}


/*
 * Function to receive the file from the server
 */
void receive_file(const char *storePath) {
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    size = read(client_socket, (unsigned char *)&fileSize, 4);
    if (size != 4) {
        printf("file size error!\n");
        return;
    }
    printf("file size:%d\n", fileSize);

    if ((size = write(client_socket, "OK", 2)) < 0) {
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
        size = read(client_socket, fileBuf, sizeof(fileBuf));
        if (size <= 0) {
            perror("read");
            fclose(fp);
            close(client_socket);
            exit(1);
        }

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
    }

    fclose(fp);
}

/*
 * Function to send the file to the server
 */
void send_file(const char *sourcePath) {
    FILE *fp = NULL;
    unsigned int fileSize;
    int size, netSize;
    char buf[10];

    fp = fopen(sourcePath, "r");
    if (fp == NULL) {
        perror("fopen");
        close(client_socket);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (write(client_socket, (unsigned char *)&fileSize, 4) != 4) {
        perror("write");
        close(client_socket);
        fclose(fp);
        exit(1);
    }

    if ((size = read(client_socket, buf, 2)) != 2 || strncmp(buf, "OK", 2) != 0) {
        perror("read");
        close(client_socket);
        fclose(fp);
        exit(1);
    }

    while (fileSize > 0) {
        size = fread(fileBuf, 1, sizeof(fileBuf), fp);
        if (size <= 0) {
            perror("read");
            fclose(fp);
            close(client_socket);
            exit(1);
        }

        unsigned int size2 = 0;
        while (size2 < size) {
            if ((netSize = write(client_socket, fileBuf + size2, size - size2)) < 0) {
                perror("write");
                fclose(fp);
                close(client_socket);
                exit(1);
            }
            size2 += netSize;
        }
        fileSize -= size;
    }

    fclose(fp);
}


/*
 * Function to handle client based on mode
 */
void handle_client_mode(char mode, const char *ip) {
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

    char sourcePath[256];
    char storePath[256];

    if (mode == 'r') {
        // Read source path from standard input
        printf("Enter source path (Ctrl+C to exit): ");
        if (fgets(sourcePath, sizeof(sourcePath), stdin) == NULL) {
            exit(0);  // Break if EOF (Ctrl+C)
        }
        sourcePath[strcspn(sourcePath, "\n")] = '\0';  // Remove newline character

        // Read store path from standard input
        printf("Enter store path: ");
        if (fgets(storePath, sizeof(storePath), stdin) == NULL) {
            exit(0);  // Break if EOF (Ctrl+C)
        }
        storePath[strcspn(storePath, "\n")] = '\0';  // Remove newline character

        // Request and receive file from the server
        send_file_paths(sourcePath, storePath);
        receive_file(storePath);
    } else if (mode == 's') {
        // Read source path from standard input
        printf("Enter source path (Ctrl+C to exit): ");
        if (fgets(sourcePath, sizeof(sourcePath), stdin) == NULL) {
            exit(0);  // Break if EOF (Ctrl+C)
        }
        sourcePath[strcspn(sourcePath, "\n")] = '\0';  // Remove newline character

        // Read store path from standard input
        printf("Enter store path: ");
        if (fgets(storePath, sizeof(storePath), stdin) == NULL) {
            exit(0);  // Break if EOF (Ctrl+C)
        }
        storePath[strcspn(storePath, "\n")] = '\0';  // Remove newline character

        // Send file to the server
        send_file_paths(sourcePath, storePath);
        send_file(sourcePath);
    } else {
        printf("Invalid mode. Use -r for receiving or -s for sending.\n");
    }

    close(client_socket);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <-r/-s> <ip_address>\n", argv[0]);
        return -1;
    }

    char mode = argv[1][1];

    if (mode != 'r' && mode != 's') {
        printf("Invalid mode. Use -r for receiving or -s for sending.\n");
        return -1;
    }

    handle_client_mode(mode, argv[2]);

    return 0;
}

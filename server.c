#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept a connection (1st step of the handshake)
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Send acknowledgment (2nd step of the handshake)
    send(client_socket, "ACK", 3, 0);

    // Receive acknowledgment (3rd step of the handshake)
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("Received acknowledgment from client: %s\n", buffer);

    // Close sockets
    close(client_socket);
    close(server_socket);

    return 0;
}
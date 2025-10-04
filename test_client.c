#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 80
#define SERVER_ADDR "127.0.0.1"   // loopback for local testing
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4/IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server on %s:%d\n", SERVER_ADDR, SERVER_PORT);

    // Send a message
    char *hello = "Hello from client!";
    send(sock, hello, strlen(hello), 0);
    printf("Message sent: %s\n", hello);

    // // Receive a response (optional)
    // int valread = read(sock, buffer, BUFFER_SIZE);
    // if (valread > 0) {
    //     buffer[valread] = '\0';
    //     printf("Server response: %s\n", buffer);
    // }

    // Close socket
    close(sock);
    return 0;
}

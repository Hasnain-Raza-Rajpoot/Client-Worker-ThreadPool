#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// helper to send data safely
ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8000);  // same as server_port

    // Convert IPv4 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server!\n");
    printf("Available commands: SIGNUP <username> <password>, LOGIN <username> <password>, UPLOAD <filename>, DOWNLOAD <filename>, DELETE <filename>, LIST\n");

    while (1) {
        printf("Loop start\n");
        char input[1024];
        printf("\nEnter command: ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // remove trailing newline
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;
        if (strcmp(input, "quit") == 0) break;

        int len = strlen(input) + 1;  // include null terminator
        if (send_all(sock, &len, sizeof(int)) <= 0) {
            perror("Send length failed");
            break;
        }
        if (send_all(sock, input, len) <= 0) {
            perror("Send data failed");
            break;
        }

        printf("Command sent: %s\n", input);

        // // Optional: wait for response from server
        // char buffer[1024];
        // int n = recv(sock, buffer, sizeof(buffer)-1, 0);
        // if (n > 0) {
        //     buffer[n] = '\0';
        //     printf("Server response: %s\n", buffer);
        // } else {
        //     printf("No response (server may close connection)\n");
        // }
        printf("Loop end\n");
    }

    close(sock);
    return 0;
}

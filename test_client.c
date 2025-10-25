#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdbool.h>

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

// helper to receive data safely
ssize_t recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *p = buf;
    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

// extract first word from string
char* get_first_word(const char* str) {
    char* space = strchr(str, ' ');
    if (space) {
        size_t len = space - str;
        char* word = malloc(len + 1);
        strncpy(word, str, len);
        word[len] = '\0';
        return word;
    }
    return strdup(str);
}

// extract filename from command
char* get_filename(const char* str) {
    char* space = strchr(str, ' ');
    if (space) {
        space++; // skip the space
        char* end = strchr(space, ' ');
        if (end) {
            size_t len = end - space;
            char* filename = malloc(len + 1);
            strncpy(filename, space, len);
            filename[len] = '\0';
            return filename;
        }
        return strdup(space);
    }
    return NULL;
}

void handle_upload(int sock, const char* filename) {
    // Open the file
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open file for upload");
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("Uploading file: %s (size: %zu bytes)\n", filename, file_size);

    // Send file size
    if (send_all(sock, &file_size, sizeof(size_t)) <= 0) {
        perror("Failed to send file size");
        fclose(fp);
        return;
    }

    // Read and send file content
    char* buffer = malloc(file_size);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(fp);
        return;
    }

    size_t read_bytes = fread(buffer, 1, file_size, fp);
    fclose(fp);

    if (read_bytes != file_size) {
        fprintf(stderr, "Failed to read complete file\n");
        free(buffer);
        return;
    }

    if (send_all(sock, buffer, file_size) <= 0) {
        perror("Failed to send file content");
        free(buffer);
        return;
    }

    free(buffer);
    printf("File upload data sent successfully\n");
}

void handle_download(int sock, const char* filename) {
    printf("Waiting for download response...\n");
    
    // Server will send response through normal flow
    // File content will be in the response
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    bool authenticated = false;

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
    printf("Available commands:\n");
    printf("  SIGNUP <username> <password>\n");
    printf("  LOGIN <username> <password>\n");
    printf("  UPLOAD <filename>\n");
    printf("  DOWNLOAD <filename>\n");
    printf("  DELETE <filename>\n");
    printf("  LIST\n");
    printf("  quit\n\n");

    while (1) {
        char input[1024];
        printf("Enter command: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) break;

        // remove trailing newline
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;
        if (strcmp(input, "quit") == 0) break;

        // Get command type
        char* cmd = get_first_word(input);
        
        // Send command length and command string
        int len = strlen(input) + 1;  // include null terminator
        if (send_all(sock, &len, sizeof(int)) <= 0) {
            perror("Send length failed");
            free(cmd);
            break;
        }
        if (send_all(sock, input, len) <= 0) {
            perror("Send data failed");
            free(cmd);
            break;
        }

        printf("Command sent: %s\n", input);

        // Handle special cases for UPLOAD
        if (strcmp(cmd, "UPLOAD") == 0) {
            char* filename = get_filename(input);
            if (filename) {
                handle_upload(sock, filename);
                free(filename);
            } else {
                fprintf(stderr, "Invalid UPLOAD command format\n");
            }
        }
        
        // Check if this was authentication command
        if (strcmp(cmd, "SIGNUP") == 0 || strcmp(cmd, "LOGIN") == 0) {
            // After sending auth command, wait briefly for processing
            // In a real implementation, you'd wait for explicit response
            printf("Authentication command sent. You can now use file commands.\n");
            authenticated = true;
        }

        free(cmd);
        
        // Small delay to let server process
        usleep(100000); // 100ms
    }

    close(sock);
    printf("Connection closed.\n");
    return 0;
}
#define _POSIX_C_SOURCE 200809L
#include "utility.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sys/socket.h>

// Global variable definitions
int server_fd;
Queue* ClntQue;
Queue* TaskQue;
FileLockTable* GlobalFileLockTable;

pthread_mutex_t ClientQueMutuex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ClientQueCV = PTHREAD_COND_INITIALIZER;

pthread_mutex_t TaskQueMutuex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t TaskQueCV = PTHREAD_COND_INITIALIZER;

void print_client(const void* data){
    client Gahak = *(client*)(data);
    printf("\nClient Information\n");
    printf("\nName: %s\nClient FD: %d\nUser-ID: %d\n",Gahak.username,Gahak.client_fd,Gahak.user_id);
}

void print_task(const void* data){
    task kaam = *(task*)(data);
    printf("\nTask Information\n");
    print_client(&kaam.Gahak);
    printf("\nCmd: %d\nFilename: %s\nPayload: %s\n Payload_Length: %zu\nStatus: %d\nDone: %d\nResponse: %s\nResponse_len: %zu\n",kaam.cmd,kaam.filename,kaam.payload,kaam.payload_len,kaam.status,kaam.done,kaam.resp,kaam.resp_len);
}

void print_int(const void* data) {
    printf("%d", *(int*)data);
}

void print_string(const void* data) {
    printf("\"%s\"", (char*)data);
}

void print_float(const void* data) {
    printf("%.2f", *(float*)data);
}

char* firstWord(char* Stmt){
    char *spacePos = strchr(Stmt, ' ');
    char *firstWord = NULL;
    if(spacePos){
        size_t len = spacePos-Stmt;
        firstWord = malloc(len+1);
        strncpy(firstWord, Stmt, len);
        firstWord[len] = '\0';
        memmove(Stmt, spacePos + 1, strlen(spacePos + 1) + 1);
    }
    else{
        firstWord = strdup(Stmt);
        Stmt[0]='\0';
    }
    return firstWord;
}


int countDigits(int n) {
    if (n == 0)
        return 1;
    return (int)log10(abs(n)) + 1;
}

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

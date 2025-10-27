#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include <arpa/inet.h>
#include "queue.c"
#include "storage.c"
#include "client.h"
#include "task.h"
#include<math.h>
#include "file_locks.c"

#define ThreadPoolSize 4
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
    printf("\nCmd: %d\nFilename: %s\nPayload: %s\n Payload_Length: %d\nStatus: %d\nDone: %d\nResponse: %s\nResponse_len: %d\n",kaam.cmd,kaam.filename,kaam.payload,kaam.payload_len,kaam.status,kaam.done,kaam.resp,kaam.resp_len);
}

void print_int(const void* data) {
    printf("%d", *(int*)data);
}

// char g_db_path[256];

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
    return (int)log10(fabs(n)) + 1;
}
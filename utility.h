#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include <arpa/inet.h>
#include "queue.c"

Queue* ClntQue;
pthread_mutex_t QueMutuex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t QueCV = PTHREAD_COND_INITIALIZER;


void print_int(const void* data) {
    printf("%d", *(int*)data);
}

char g_db_path[256];

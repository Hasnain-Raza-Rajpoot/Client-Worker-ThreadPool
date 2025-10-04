#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include <arpa/inet.h>
#include "queue.c"

#define ThreadPoolSize 4

Queue* ClntQue;
pthread_mutex_t QueMutuex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t QueCV = PTHREAD_COND_INITIALIZER;



void print_int(const void* data) {
    printf("%d", *(int*)data);
}

void print_string(const void* data) {
    printf("\"%s\"", (char*)data);
}

void print_float(const void* data) {
    printf("%.2f", *(float*)data);
}
#ifndef TASK_H
#define TASK_H
#include<stdlib.h>
#include<pthread.h>
#include"client.h"

typedef enum CMD{
    UPLOAD,DOWNLOAD,DELETE,LIST
}CMD;

typedef struct task {
    CMD cmd;                // UPLOAD/DOWNLOAD/DELETE/LIST
    client Gahak;
    char* filename;     // empty for LIST
    char* payload; size_t payload_len;   // for UPLOAD/DOWNLOAD bodies
    // result
    int status;             // 0 ok, errno-style on failure
    char *resp; size_t resp_len;
    // completion
    pthread_mutex_t m;
    pthread_cond_t  cv;
    int done;               // 0/1
} task;

#endif
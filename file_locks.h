#ifndef FILE_LOCKS_H
#define FILE_LOCKS_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_FILE_LOCKS 1024
#define FILEPATH_MAX 512

typedef struct FileLock {
    char filepath[FILEPATH_MAX];
    pthread_mutex_t lock;
    int ref_count;           
    bool in_use;             
} FileLock;

typedef struct FileLockTable {
    FileLock locks[MAX_FILE_LOCKS];
    pthread_mutex_t table_lock;
    int count;                 
} FileLockTable;


FileLockTable* file_lock_table_init();

pthread_mutex_t* acquire_file_lock(FileLockTable* table, const char* filepath);

void release_file_lock(FileLockTable* table, const char* filepath);

void file_lock_table_destroy(FileLockTable* table);

void file_lock_table_print(FileLockTable* table);

#endif
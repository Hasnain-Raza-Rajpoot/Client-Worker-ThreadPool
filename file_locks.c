#include "file_locks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileLockTable* file_lock_table_init() {
    FileLockTable* table = (FileLockTable*)malloc(sizeof(FileLockTable));
    if (!table) {
        perror("Failed to allocate file lock table");
        return NULL;
    }
    
    if (pthread_mutex_init(&table->table_lock, NULL) != 0) {
        perror("Failed to initialize table lock");
        free(table);
        return NULL;
    }
    
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        table->locks[i].filepath[0] = '\0';
        table->locks[i].ref_count = 0;
        table->locks[i].in_use = false;
        
        if (pthread_mutex_init(&table->locks[i].lock, NULL) != 0) {
            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&table->locks[j].lock);
            }
            pthread_mutex_destroy(&table->table_lock);
            free(table);
            return NULL;
        }
    }
    
    table->count = 0;
    
    printf("File lock table initialized (capacity: %d)\n", MAX_FILE_LOCKS);
    return table;
}

static int find_lock_slot(FileLockTable* table, const char* filepath, bool create) {
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        if (table->locks[i].in_use && 
            strcmp(table->locks[i].filepath, filepath) == 0) {
            return i;
        }
    }
    if (create) {
        for (int i = 0; i < MAX_FILE_LOCKS; i++) {
            if (!table->locks[i].in_use) {
                return i;  
            }
        }
    }
    
    return -1;  
}

pthread_mutex_t* acquire_file_lock(FileLockTable* table, const char* filepath) {
    if (!table || !filepath) {
        fprintf(stderr, "acquire_file_lock: NULL argument\n");
        return NULL;
    }
    
    pthread_mutex_lock(&table->table_lock);
    
    int slot = find_lock_slot(table, filepath, true);
    
    if (slot == -1) {
        pthread_mutex_unlock(&table->table_lock);
        fprintf(stderr, "File lock table full! Cannot lock: %s\n", filepath);
        return NULL;
    }
    
    FileLock* file_lock = &table->locks[slot];
    
    if (!file_lock->in_use) {
        strncpy(file_lock->filepath, filepath, FILEPATH_MAX - 1);
        file_lock->filepath[FILEPATH_MAX - 1] = '\0';
        file_lock->ref_count = 0;
        file_lock->in_use = true;
        table->count++;
        printf("[LOCK] Created new lock for: %s (slot %d)\n", filepath, slot);
    }
    
    file_lock->ref_count++;
    
    printf("[LOCK] Acquiring lock for: %s (ref_count: %d)\n", 
           filepath, file_lock->ref_count);
    
    pthread_mutex_unlock(&table->table_lock);
    
    pthread_mutex_lock(&file_lock->lock);
    
    printf("[LOCK] Lock acquired for: %s\n", filepath);
    
    return &file_lock->lock;
}

void release_file_lock(FileLockTable* table, const char* filepath) {
    if (!table || !filepath) {
        fprintf(stderr, "release_file_lock: NULL argument\n");
        return;
    }
    
    pthread_mutex_lock(&table->table_lock);
    
    int slot = find_lock_slot(table, filepath, false);
    
    if (slot == -1) {
        pthread_mutex_unlock(&table->table_lock);
        fprintf(stderr, "release_file_lock: Lock not found for: %s\n", filepath);
        return;
    }
    
    FileLock* file_lock = &table->locks[slot];
    
    pthread_mutex_unlock(&file_lock->lock);
    
    printf("[LOCK] Lock released for: %s\n", filepath);
    
    file_lock->ref_count--;
    
    printf("[LOCK] Reference count for %s: %d\n", filepath, file_lock->ref_count);
    
    if (file_lock->ref_count == 0) {
        file_lock->in_use = false;
        file_lock->filepath[0] = '\0';
        table->count--;
        printf("[LOCK] Removed lock for: %s (slot %d freed)\n", filepath, slot);
    }
    
    pthread_mutex_unlock(&table->table_lock);
}

void file_lock_table_destroy(FileLockTable* table) {
    if (!table) return;
    
    printf("Destroying file lock table...\n");
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        pthread_mutex_destroy(&table->locks[i].lock);
    }
    
    pthread_mutex_destroy(&table->table_lock);
    free(table);
    
    printf("File lock table destroyed\n");
}

void file_lock_table_print(FileLockTable* table) {
    if (!table) return;
    
    pthread_mutex_lock(&table->table_lock);
    
    printf("\n=== File Lock Table Status ===\n");
    printf("Active locks: %d / %d\n", table->count, MAX_FILE_LOCKS);
    
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        if (table->locks[i].in_use) {
            printf("  [%d] %s (ref_count: %d)\n", 
                   i, table->locks[i].filepath, table->locks[i].ref_count);
        }
    }
    printf("==============================\n\n");
    
    pthread_mutex_unlock(&table->table_lock);
}
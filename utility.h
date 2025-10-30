#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include <arpa/inet.h>
#include "queue.h"
#include "storage.h"
#include "client.h"
#include "task.h"
#include<math.h>
#include <stdlib.h> // For abs and strdup
#include "file_locks.h"

#define ThreadPoolSize 4
extern int server_fd;

extern Queue* ClntQue;
extern Queue* TaskQue;
extern FileLockTable* GlobalFileLockTable;

extern pthread_mutex_t ClientQueMutuex;
extern pthread_cond_t ClientQueCV;

extern pthread_mutex_t TaskQueMutuex;
extern pthread_cond_t TaskQueCV;


void print_client(const void* data);

void print_task(const void* data);

void print_int(const void* data);

void print_string(const void* data);

void print_float(const void* data);

char* firstWord(char* Stmt);

int countDigits(int n);

ssize_t recv_all(int sock, void *buf, size_t len);
ssize_t send_all(int sock, const void *buf, size_t len);
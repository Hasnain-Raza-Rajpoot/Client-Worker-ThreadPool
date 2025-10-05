#ifndef CLIENT_H
#define CLIENT_H
#include<stdlib.h>

typedef struct client {
    int client_fd;          
    int user_id;
    char username[256];
} client;

#endif
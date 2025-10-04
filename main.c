#include "utility.h"

#define server_port 80


int main(){
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    if((server_fd = socket(AF_INET,SOCK_STREAM,0))==0){
        perror("Failed to start the server\n");
        exit(0);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);

    if(bind(server_fd,(struct sockaddr*)&address,sizeof(address))<0){
        perror("Failed to bind the server\n");
        exit(0);
    }

    if(listen(server_fd,3)<0){
        perror("Server failed to listen\n");
        exit(0);
    }
    ClntQue = queue_init(sizeof(int));
    while(1){
        client_fd=accept(server_fd,(struct sockaddr*)&address,&addrlen);
        if(client_fd<0){
            perror("Failed to load the client_fd");
            continue;
        }
        pthread_mutex_lock(&QueMutuex);
        queue_enqueue(ClntQue,&client_fd);
        pthread_cond_signal(&QueCV);
        pthread_mutex_unlock(&QueMutuex);
        printf("\n\n\n--------------------------------------\n\n");
        queue_print(ClntQue,print_int);
        printf("\n\n\n--------------------------------------\n\n");
    }

    printf("The value %d\n",global);
    queue_destroy(ClntQue);
    return 0;
}

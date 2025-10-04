#include "utility.h"

#define server_port 8000
atomic_int ServerRunning = 1;


void* handle_client(void *arg){
    while(1){
        pthread_mutex_lock(&QueMutuex);
        if(!queue_is_empty(ClntQue) and ServerRunning){
            pthread_cond_wait(&QueCV);
        }

        if(!ServerRunning and queue_is_empty(ClntQue)){
            pthread_mutex_unlock(&QueMutuex);
            break;    
        }

        int client_fd;
        queue_dequeue(ClntQue,&client_fd);
        pthread_mutex_unlock(&QueMutuex);
    }
    return NULL;
}

void* handle_signint(void *arg){
    ServerRunning = 0;
    pthread_cond_broadcast(&QueCV);
}

int main(){
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    pthread_t ClienThreadPool[ThreadPoolSize];

    signal(SIGINT,handle_signint);

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
    for(int i=0;i<ThreadPoolSize;i++){
        pthread_create(&ClienThreadPool[i],NULL,&handle_client,NULL);
    }
    while(ServerRunning){
        client_fd=accept(server_fd,(struct sockaddr*)&address,&addrlen);
        if(client_fd<0){
            if(!ServerRunning)
                break;
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
    close(server_fd);
    for(int i=0;i<ThreadPoolSize;i++){
        pthread_join(ClienThreadPool[i],NULL);
    }

    printf("The value %d\n",global);
    queue_destroy(ClntQue);
    return 0;
}

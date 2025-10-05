#include "utility.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <signal.h>

#define server_port 8000
atomic_int ServerRunning = 1;


bool upload(task* aikKaam, char* filename){
    aikKaam->cmd = UPLOAD;
    aikKaam->filename = filename;

    if(recv(aikKaam->Gahak.client_fd,&(aikKaam->payload_len),sizeof(size_t),0)>0){
        aikKaam->payload = malloc(aikKaam->payload_len*sizeof(char));
        if(recv(aikKaam->Gahak.client_fd,aikKaam->payload,aikKaam->payload_len,0)>0){
            return true;
        }
    }
    return false;
}

bool download(task* aikKaam, char* filename){
    aikKaam->cmd = DOWNLOAD;
    aikKaam->filename = filename;
    return true;
}

bool delete(task* aikKaam, char* filename){
    aikKaam->cmd = DELETE;
    aikKaam->filename = filename;
    return true;
}


bool handleCmd(task* kaam,char * Cmd){
    char* what2do = firstWord(Cmd);
    char * filename = firstWord(Cmd);

    return ((strcmp(what2do,"UPLOAD") && upload(kaam,filename)) 
            || (strcmp(what2do,"DOWNLOAD") && download(kaam,filename)) 
            || (strcmp(what2do,"DELETE") && delete(kaam,filename))
            || (strcmp(what2do,"LIST") && (kaam->cmd = LIST)));

}

void* handle_client(void *arg){
    while(1){
        pthread_mutex_lock(&ClientQueMutuex);
        if(queue_is_empty(ClntQue) && ServerRunning){
            pthread_cond_wait(&ClientQueCV,&ClientQueMutuex);
        }
        if(!ServerRunning && queue_is_empty(ClntQue)){
            pthread_mutex_unlock(&ClientQueMutuex);
            break;    
        }
        client Gahak;
        queue_dequeue(ClntQue,&Gahak);
        pthread_mutex_unlock(&ClientQueMutuex);
        int CmdSize;
        char * Cmd;
        if(recv(Gahak.client_fd,&CmdSize,sizeof(int),0)>0){
            Cmd = malloc(CmdSize);
            if(recv(Gahak.client_fd,Cmd,CmdSize,0)>0){
                task kaam;
                kaam.Gahak = Gahak;        
                if(handleCmd(&kaam,Cmd))
                {
                    pthread_mutex_lock(&TaskQueMutuex);
                    queue_enqueue(TaskQue,&kaam);
                    queue_print(TaskQue,print_task);
                    pthread_cond_signal(&TaskQueCV);
                    pthread_mutex_unlock(&TaskQueMutuex);
                }
                else{
                    // send invalid command response
                    close(Gahak.client_fd);
                }
            }
        }
    }
    return NULL;
}

void handle_signint(int signum){
    ServerRunning = 0;
    close(server_fd);
    pthread_cond_broadcast(&ClientQueCV);
}

int main(){
    if(init_db("DB/users.db")!=0){
        perror("\n Can not init the db\n");
        return 0;
    }

    int client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    pthread_t ClienThreadPool[ThreadPoolSize];

    signal(SIGINT,handle_signint);

    if((server_fd = socket(AF_INET,SOCK_STREAM,0))==0){
        perror("Failed to start the server");
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
        perror("Server failed to listen");
        exit(0);
    }

    ClntQue = queue_init(sizeof(client));
    TaskQue = queue_init(sizeof(task));

    for(int i=0;i<ThreadPoolSize;i++){
        pthread_create(&ClienThreadPool[i],NULL,&handle_client,NULL);
    }
    while(ServerRunning){
        if((client_fd=accept(server_fd,(struct sockaddr*)&address,&addrlen))<0){
            if(!ServerRunning)
                break;
            perror("Failed to load the client_fd");
            continue;
        }
        int CmdSize;
        char * Cmd;
        bool jaanDeyo=false;

        client Gahak;

        if(recv(client_fd,&CmdSize,sizeof(int),0)>0){
            Cmd = malloc(CmdSize);
            if(recv(client_fd,Cmd,CmdSize,0)>0){
                printf("\nThe command is in the %s\n",Cmd);
                char* what2do = firstWord(Cmd);
                if(strcmp(what2do,"SIGNUP")==0){
                    char* username = firstWord(Cmd);
                    char* password = firstWord(Cmd);
                    printf("\nThe username is %s\n",username);
                    printf("The password is %s\n",password);
                    jaanDeyo=signup(username,password);
                    int user_id = get_user_id(username);
                    if(user_id!=-1){
                        char * foldername=malloc(strlen(username)+10);
                        memset(foldername,'\0',strlen(foldername));
                        strncpy(foldername,username,strlen(username));
                        printf("The current foldername is %s\n",foldername);
                        char str_user_id[2];
                        sprintf(str_user_id, "%d", user_id);
                        strcat(foldername,str_user_id);
                        if(mkdir(foldername,0777)!=0){
                            perror("Can not create the folder\n");
                            jaanDeyo = false;
                        }    
                        
                        Gahak.client_fd = client_fd;
                        Gahak.user_id = user_id;
                        memset(Gahak.username,'\0',sizeof(Gahak.username));
                        strncpy(Gahak.username, username, strlen(username));
                        free(foldername);
                    }
                    else{
                        jaanDeyo = false;  
                    }
                    free(username);
                    free(password);
                }
                else if(strcmp(what2do,"LOGIN")==0){
                    char* username = firstWord(Cmd);
                    char* password = firstWord(Cmd);
                    strncpy(Gahak.username, username, strlen(username));
                    Gahak.client_fd = client_fd;
                    Gahak.user_id = get_user_id(username);
                    jaanDeyo=login(username,password);
                }
            }
        }
        if(jaanDeyo){
            pthread_mutex_lock(&ClientQueMutuex);
            queue_enqueue(ClntQue,&Gahak);
            printf("\n\n\n--------------------------------------\n\n");
            queue_print(ClntQue,print_client);
            printf("--------------------------------------\n\n");
            pthread_cond_signal(&ClientQueCV);
            pthread_mutex_unlock(&ClientQueMutuex);
        } 
        else{
            printf("Closing the client_fd");
            close(client_fd);
        }

    }
    close(server_fd);
    for(int i=0;i<ThreadPoolSize;i++){
        pthread_join(ClienThreadPool[i],NULL);
    }

    printf("The value %d\n",global);
    queue_destroy(ClntQue);
    return 0;
}

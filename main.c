#include "utility.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <signal.h>
#include <dirent.h> 

#define server_port 8000
atomic_int ServerRunning = 1;

void execute_task(task* kaam) {
    // Build the user's directory path
    char user_dir[512];
    int dc = countDigits(kaam->Gahak.user_id);
    char* str_user_id = malloc(dc + 1);
    sprintf(str_user_id, "%d", kaam->Gahak.user_id);
    
    snprintf(user_dir, sizeof(user_dir), "%s%s/", kaam->Gahak.username, str_user_id);
    free(str_user_id);
    
    switch(kaam->cmd) {
        case UPLOAD: {
            printf("Worker: Uploading file %s\n", kaam->filename);
            
            // Build full file path
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);
            
            // Write payload to file
            FILE* fp = fopen(filepath, "wb");
            if(fp == NULL) {
                perror("Failed to open file for writing");
                kaam->status = -1;
                kaam->resp = strdup("UPLOAD FAILED: Cannot create file");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            
            size_t written = fwrite(kaam->payload, 1, kaam->payload_len, fp);
            fclose(fp);
            
            if(written != kaam->payload_len) {
                kaam->status = -1;
                kaam->resp = strdup("UPLOAD FAILED: Incomplete write");
            } else {
                kaam->status = 0;
                kaam->resp = strdup("UPLOAD SUCCESS");
            }
            kaam->resp_len = strlen(kaam->resp) + 1;
            
            // Free the payload after writing
            free(kaam->payload);
            kaam->payload = NULL;
            break;
        }
        
        case DOWNLOAD: {
            printf("Worker: Downloading file %s\n", kaam->filename);
            
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);
            
            // Read file from disk
            FILE* fp = fopen(filepath, "rb");
            if(fp == NULL) {
                perror("Failed to open file for reading");
                kaam->status = -1;
                kaam->resp = strdup("DOWNLOAD FAILED: File not found");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            
            // Get file size
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            // Allocate buffer and read file
            kaam->resp = malloc(file_size);
            if(kaam->resp == NULL) {
                fclose(fp);
                kaam->status = -1;
                kaam->resp = strdup("DOWNLOAD FAILED: Memory allocation error");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            
            size_t read_bytes = fread(kaam->resp, 1, file_size, fp);
            fclose(fp);
            
            if(read_bytes != file_size) {
                free(kaam->resp);
                kaam->status = -1;
                kaam->resp = strdup("DOWNLOAD FAILED: Read error");
                kaam->resp_len = strlen(kaam->resp) + 1;
            } else {
                kaam->status = 0;
                kaam->resp_len = file_size;
            }
            break;
        }
        
        case DELETE: {
            printf("Worker: Deleting file %s\n", kaam->filename);
            
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);
            
            if(unlink(filepath) == 0) {
                kaam->status = 0;
                kaam->resp = strdup("DELETE SUCCESS");
            } else {
                perror("Failed to delete file");
                kaam->status = -1;
                kaam->resp = strdup("DELETE FAILED: File not found or permission denied");
            }
            kaam->resp_len = strlen(kaam->resp) + 1;
            break;
        }
        
        case LIST: {
            printf("Worker: Listing files for user %s\n", kaam->Gahak.username);
            
            DIR* dir = opendir(user_dir);
            if(dir == NULL) {
                perror("Failed to open directory");
                kaam->status = -1;
                kaam->resp = strdup("LIST FAILED: Cannot open directory");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            
            // Build file list
            char file_list[4096] = "FILES:\n";
            struct dirent* entry;
            int count = 0;
            
            while((entry = readdir(dir)) != NULL) {
                // Skip . and ..
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                    
                strcat(file_list, entry->d_name);
                strcat(file_list, "\n");
                count++;
            }
            closedir(dir);
            
            if(count == 0) {
                strcpy(file_list, "No files found");
            }
            
            kaam->status = 0;
            kaam->resp = strdup(file_list);
            kaam->resp_len = strlen(kaam->resp) + 1;
            break;
        }
        
        default:
            kaam->status = -1;
            kaam->resp = strdup("UNKNOWN COMMAND");
            kaam->resp_len = strlen(kaam->resp) + 1;
            break;
    }
}

void* handle_worker(void* arg) {
    while(1) {
        pthread_mutex_lock(&TaskQueMutuex);
        while(queue_is_empty(TaskQue) && ServerRunning) {
            pthread_cond_wait(&TaskQueCV, &TaskQueMutuex);
        }
        if(!ServerRunning && queue_is_empty(TaskQue)) {
            pthread_mutex_unlock(&TaskQueMutuex);
            break;
        }
        
        task kaam;
        queue_dequeue(TaskQue, &kaam);
        pthread_mutex_unlock(&TaskQueMutuex);
        
        printf("\nWorker thread processing task for user: %s\n", kaam.Gahak.username);
        
        execute_task(&kaam);
        
        pthread_mutex_lock(&kaam.m);
        kaam.done = 1;
        pthread_cond_signal(&kaam.cv);
        pthread_mutex_unlock(&kaam.m);
        
        pthread_mutex_destroy(&kaam.m);
        pthread_cond_destroy(&kaam.cv);
    }
    return NULL;
}

bool signup_login(char* Cmd, client* Gahak){
    bool jaanDeyo=false;
    printf("\nThe command is in theeeeee %s\n",Cmd);
    char* what2do = firstWord(Cmd);
    char* username = firstWord(Cmd);
    char* password = firstWord(Cmd);
    printf("\nWhat2Do %s\n",what2do);
    printf("\nThe username is %s\n",username);
    printf("The password is %s\n",password);
    if(strcmp(what2do,"SIGNUP")==0){
        printf("Going to check for the sing up\n");
        jaanDeyo=signup(username,password);
        int user_id = get_user_id(username);
        if(user_id!=-1){
            char * foldername=malloc(strlen(username)+10);
            memset(foldername,'\0',strlen(foldername));
            strncpy(foldername,username,strlen(username));
            printf("The current foldername is %s\n",foldername);
            int dc = countDigits(user_id);
            char* str_user_id = malloc(dc+1);

            sprintf(str_user_id, "%d", user_id);
            strcat(foldername,str_user_id);
            if(mkdir(foldername,0777)!=0){
                perror("Can not create the folder\n");
                jaanDeyo = false;
            }    
            Gahak->user_id = user_id;
            memset(Gahak->username,'\0',sizeof(Gahak->username));
            strncpy(Gahak->username, username, strlen(username));
            free(foldername);
            free(str_user_id);
        }
        else{
            jaanDeyo = false;  
        }
    }
    else if(strcmp(what2do,"LOGIN")==0){
        printf("Going to check for the login\n");
        strncpy(Gahak->username, username, strlen(username));
        Gahak->user_id = get_user_id(username);
        jaanDeyo=login(username,password);
    }
    free(username);
    free(password);
    printf("The singup login function returns: %d\n",jaanDeyo);
    return jaanDeyo;
}

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

    return (((strcmp(what2do,"UPLOAD") == 0) && upload(kaam,filename)) 
            || ((strcmp(what2do,"DOWNLOAD") == 0) && download(kaam,filename)) 
            || ((strcmp(what2do,"DELETE") == 0)  && delete(kaam,filename))
            || ((strcmp(what2do,"LIST") == 0) && (kaam->cmd = LIST)));

}

void* handle_client(void *arg){
    while(1){
        printf("\nChl rha hai\n");
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
        bool jaanDeyo=false;
        if(recv(Gahak.client_fd,&CmdSize,sizeof(int),0)>0){
            Cmd = malloc(CmdSize);
            if(recv(Gahak.client_fd,Cmd,CmdSize,0)>0){
                printf("\nGoing to check for he signup login\n");
                jaanDeyo=signup_login(Cmd,&Gahak);
                print_client(&Gahak);
            }
        }
        free(Cmd);
        task kaam={};
        while(jaanDeyo){
            if(recv(Gahak.client_fd,&CmdSize,sizeof(int),0)>0){
                Cmd = malloc(CmdSize);
                if(recv(Gahak.client_fd,Cmd,CmdSize,0)>0){
                    kaam.Gahak = Gahak;        
                    int i = handleCmd(&kaam,Cmd);
                    printf("\nInavalid command\n");
                    if(i)
                    {
                        printf("In the handle client\n");
                        if(pthread_mutex_init(&kaam.m,NULL)!=0){
                            perror("Failed to initialize task mutex for clientfd");
                        }
                        if(pthread_cond_init(&kaam.cv,NULL)!=0){
                            perror("Failed to initialize task conditional variables for clientfd");
                        }
                        pthread_mutex_lock(&TaskQueMutuex);
                        queue_enqueue(TaskQue,&kaam);
                        queue_print(TaskQue,print_task);
                        pthread_cond_signal(&TaskQueCV);
                        pthread_mutex_unlock(&TaskQueMutuex);
                        
                        pthread_mutex_lock(&kaam.m);
                        while(!kaam.done){
                            pthread_cond_wait(&kaam.cv,&kaam.m);
                        }
                        pthread_mutex_unlock(&kaam.m);
                    }
                    else{
                        // send invalid command response
                    }
                }
                free(Cmd);
            }
        }
        close(Gahak.client_fd);
        printf("Client is freeing the user id:\n");
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
    pthread_t WorkerThreadPool[ThreadPoolSize];

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
    for(int i=0; i<ThreadPoolSize; i++){
        pthread_create(&WorkerThreadPool[i], NULL, &handle_worker, NULL);
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
        client Gahak;
        Gahak.client_fd = client_fd;
        pthread_mutex_lock(&ClientQueMutuex);
        queue_enqueue(ClntQue,&Gahak);
        printf("\n--------------------------------------\n\n");
        queue_print(ClntQue,print_client);
        printf("--------------------------------------\n\n");
        pthread_cond_signal(&ClientQueCV);
        pthread_mutex_unlock(&ClientQueMutuex);

    }
    close(server_fd);
    for(int i=0;i<ThreadPoolSize;i++){
        pthread_join(ClienThreadPool[i],NULL);
    }
    pthread_cond_broadcast(&TaskQueCV);
    for(int i=0; i<ThreadPoolSize; i++){
        pthread_join(WorkerThreadPool[i], NULL);
    }

    printf("The value %d\n",global);
    queue_destroy(ClntQue);
    queue_destroy(TaskQue);
    return 0;
}

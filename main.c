#include "utility.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <signal.h>
#include <dirent.h> 

#define server_port 8000
atomic_int ServerRunning = 1;

void execute_task(task* kaam) {
    char user_dir[512];
    int dc = countDigits(kaam->Gahak.user_id);
    char* str_user_id = malloc(dc + 1);
    sprintf(str_user_id, "%d", kaam->Gahak.user_id);
    
    snprintf(user_dir, sizeof(user_dir), "%s%s/", kaam->Gahak.username, str_user_id);
    free(str_user_id);
    
    switch(kaam->cmd) {
        case UPLOAD: {
            printf("Worker: Uploading file %s\n", kaam->filename);
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);
            printf("The filepath is %s\n",filepath);

            if (!check_quota(kaam->Gahak.user_id, kaam->payload_len)) {
                kaam->status = -1;
                
                long quota = get_user_quota(kaam->Gahak.user_id);
                long used = get_used_space(kaam->Gahak.user_id);
                long available = quota - used;
                
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg), 
                        "UPLOAD FAILED: Quota exceeded. Available: %ld bytes, Needed: %zu bytes",
                        available, kaam->payload_len);
                
                kaam->resp = strdup(error_msg);
                kaam->resp_len = strlen(kaam->resp) + 1;
                
                // Free the payload
                free(kaam->payload);
                kaam->payload = NULL;
                break;
            }

            pthread_mutex_t* file_lock = acquire_file_lock(GlobalFileLockTable, filepath);
            if (!file_lock) {
                kaam->status = -1;
                kaam->resp = strdup("UPLOAD FAILED: Could not acquire lock");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }

            bool file_exists = false;
            long old_file_size = 0;
            struct stat st;
            if (stat(filepath, &st) == 0) {
                file_exists = true;
                old_file_size = st.st_size;
                printf("File exists, size: %ld bytes. Will be overwritten.\n", old_file_size);
            }

            FILE* fp = fopen(filepath, "wb");
            if(fp == NULL) {
                perror("Failed to open file for writing");
                kaam->status = -1;
                kaam->resp = strdup("UPLOAD FAILED: Cannot create file");
                kaam->resp_len = strlen(kaam->resp) + 1;
                release_file_lock(GlobalFileLockTable, filepath);
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

                long delta;
                if (file_exists) {
                    delta = (long)kaam->payload_len - old_file_size;
                    printf("Overwriting file: delta = %ld bytes\n", delta);
                } else {
                    delta = (long)kaam->payload_len;
                }
                
                if (!update_used_space(kaam->Gahak.user_id, delta)) {
                    fprintf(stderr, "Warning: Failed to update quota after upload\n");
                }
            }
            kaam->resp_len = strlen(kaam->resp) + 1;
            free(kaam->payload);
            kaam->payload = NULL;
            release_file_lock(GlobalFileLockTable, filepath);
            break;
        }
        
        case DOWNLOAD: {
            printf("Worker: Downloading file %s\n", kaam->filename);
            
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);
            printf("The filepath is %s\n",filepath);
            pthread_mutex_t* file_lock = acquire_file_lock(GlobalFileLockTable, filepath);
            if (!file_lock) {
                kaam->status = -1;
                kaam->resp = strdup("UPLOAD FAILED: Could not acquire lock");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            FILE* fp = fopen(filepath, "rb");
            if(fp == NULL) {
                perror("Failed to open file for reading");
                kaam->status = -1;
                kaam->resp = strdup("DOWNLOAD FAILED: File not found");
                kaam->resp_len = strlen(kaam->resp) + 1;
                release_file_lock(GlobalFileLockTable, filepath);
                break;
            }
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            kaam->resp = malloc(file_size);
            if(kaam->resp == NULL) {
                fclose(fp);
                kaam->status = -1;
                kaam->resp = strdup("DOWNLOAD FAILED: Memory allocation error");
                kaam->resp_len = strlen(kaam->resp) + 1;
                release_file_lock(GlobalFileLockTable, filepath);
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
            printf("Successfully downloaded the %s from the %s\n",kaam->filename,filepath);
            release_file_lock(GlobalFileLockTable, filepath);
            break;
        }
        
        case DELETE: {
            printf("Worker: Deleting file %s\n", kaam->filename);
            
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s%s", user_dir, kaam->filename);

            pthread_mutex_t* file_lock = acquire_file_lock(GlobalFileLockTable, filepath);
            if (!file_lock) {
                kaam->status = -1;
                kaam->resp = strdup("DELETE FAILED: Could not acquire lock");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }

             struct stat st;
            long file_size = 0;
            bool file_exists = false;
            
            if (stat(filepath, &st) == 0) {
                file_size = st.st_size;
                file_exists = true;
                printf("File size: %ld bytes\n", file_size);
            }
            
            if(unlink(filepath) == 0) {
                kaam->status = 0;
                kaam->resp = strdup("DELETE SUCCESS");
                if (file_exists && !update_used_space(kaam->Gahak.user_id, -file_size)) {
                    fprintf(stderr, "Warning: Failed to update quota after delete\n");
                }
            } else {
                perror("Failed to delete file");
                kaam->status = -1;
                kaam->resp = strdup("DELETE FAILED: File not found or permission denied");
            }
            kaam->resp_len = strlen(kaam->resp) + 1;
            release_file_lock(GlobalFileLockTable, filepath);
            break;
        }
        
        case LIST: {
            printf("Worker: Listing files for user %s\n", kaam->Gahak.username);
            pthread_mutex_t* dir_lock = acquire_file_lock(GlobalFileLockTable, user_dir);
            if (!dir_lock) {
                kaam->status = -1;
                kaam->resp = strdup("LIST FAILED: Could not acquire lock");
                kaam->resp_len = strlen(kaam->resp) + 1;
                break;
            }
            DIR* dir = opendir(user_dir);
            if(dir == NULL) {
                perror("Failed to open directory");
                kaam->status = -1;
                kaam->resp = strdup("LIST FAILED: Cannot open directory");
                kaam->resp_len = strlen(kaam->resp) + 1;
                release_file_lock(GlobalFileLockTable, user_dir);
                break;
            }

             long quota = get_user_quota(kaam->Gahak.user_id);
            long used = get_used_space(kaam->Gahak.user_id);

            char file_list[4096] = "FILES:\n";
            struct dirent* entry;
            int count = 0;
            
            while((entry = readdir(dir)) != NULL) {
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                    
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s%s", user_dir, entry->d_name);
                
                struct stat st;
                long size = 0;
                if (stat(full_path, &st) == 0) {
                    size = st.st_size;
                }
                
                char file_info[256];
                snprintf(file_info, sizeof(file_info), "%s (%ld bytes)\n", entry->d_name, size);
                strcat(file_list, file_info);   
                count++;
            }
            closedir(dir);
            
            if(count == 0) {
                strcpy(file_list, "No files found");
            }
            
            kaam->status = 0;
            kaam->resp = strdup(file_list);
            kaam->resp_len = strlen(kaam->resp) + 1;
            release_file_lock(GlobalFileLockTable, user_dir);
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
        
        task* kaam;
        queue_dequeue(TaskQue, &kaam);
        pthread_mutex_unlock(&TaskQueMutuex);
        
        printf("\nWorker thread processing task for user: %s\n", kaam->Gahak.username);
        execute_task(kaam);
        pthread_mutex_lock(&kaam->m);
        kaam->done = 1;
        pthread_cond_signal(&kaam->cv);
        pthread_mutex_unlock(&kaam->m);
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
    free(what2do);
    free(username);
    free(password);
    printf("The singup login function returns: %d\n",jaanDeyo);
    return jaanDeyo;
}

bool upload(task* aikKaam, char* filename){
    aikKaam->cmd = UPLOAD;
    aikKaam->filename = filename;
    if(recv(aikKaam->Gahak.client_fd,&(aikKaam->payload_len),sizeof(size_t),0)>0){
        aikKaam->payload = malloc(aikKaam->payload_len);
        if(aikKaam->payload == NULL) {
            perror("Memory allocation failed for payload");
            return false;
        }
        if(recv_all(aikKaam->Gahak.client_fd,aikKaam->payload,aikKaam->payload_len)>0){
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

    bool result = (((strcmp(what2do,"UPLOAD") == 0) && upload(kaam,filename)) 
            || ((strcmp(what2do,"DOWNLOAD") == 0) && download(kaam,filename)) 
            || ((strcmp(what2do,"DELETE") == 0)  && delete(kaam,filename))
            || ((strcmp(what2do,"LIST") == 0) && (kaam->cmd = LIST)));
    
    free(what2do);   
    return result;
}

ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}

void* handle_client(void *arg){
    while(1){
        printf("\nClient thread waiting for connection\n");
        pthread_mutex_lock(&ClientQueMutuex);
        while(queue_is_empty(ClntQue) && ServerRunning){
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
                printf("\nGoing to check for signup/login\n");
                jaanDeyo=signup_login(Cmd,&Gahak);
                print_client(&Gahak);
                
                if(jaanDeyo) {
                    char* auth_resp = "AUTH SUCCESS";
                    size_t auth_len = strlen(auth_resp) + 1;
                    send_all(Gahak.client_fd, &auth_len, sizeof(size_t));
                    send_all(Gahak.client_fd, auth_resp, auth_len);
                } else {
                    char* auth_resp = "AUTH FAILED";
                    size_t auth_len = strlen(auth_resp) + 1;
                    send_all(Gahak.client_fd, &auth_len, sizeof(size_t));
                    send_all(Gahak.client_fd, auth_resp, auth_len);
                }
            }
            free(Cmd);
        }
        
        while(jaanDeyo){
            if(recv(Gahak.client_fd,&CmdSize,sizeof(int),0)<=0){
                printf("Client disconnected\n");
                break;
            }
            
            Cmd = malloc(CmdSize);
            if(recv(Gahak.client_fd,Cmd,CmdSize,0)<=0){
                free(Cmd);
                printf("Failed to receive command data\n");
                break;
            }
            
            printf("\nReceived command: %s\n", Cmd);
            
            task* kaam = (task*)malloc(sizeof(task));
            memset(kaam, 0, sizeof(task));
            kaam->Gahak = Gahak;
            
            int valid_cmd = handleCmd(kaam, Cmd);
            
            if(valid_cmd) {
                printf("Valid command, processing...\n");
                // kaam->m = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
                // kaam->cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
                
                if(pthread_mutex_init(&kaam->m, NULL) != 0){
                    perror("Failed to initialize task mutex");
                    free(kaam);
                    free(Cmd);
                    continue;
                }
                if(pthread_cond_init(&kaam->cv, NULL) != 0){
                    perror("Failed to initialize task cv");
                    pthread_mutex_destroy(&kaam->m);
                    free(kaam);
                    free(Cmd);
                    continue;
                }
                
                pthread_mutex_lock(&TaskQueMutuex);
                queue_enqueue(TaskQue, &kaam);
                pthread_cond_signal(&TaskQueCV);
                pthread_mutex_unlock(&TaskQueMutuex);
                
                printf("Task enqueued, waiting for worker...\n");
                
                pthread_mutex_lock(&kaam->m);
                while(!kaam->done){
                    pthread_cond_wait(&kaam->cv, &kaam->m);
                }
                pthread_mutex_unlock(&kaam->m);
                
                printf("Task completed by worker. Status: %d\n", kaam->status);
                
                if(kaam->resp && kaam->resp_len > 0) {
                    printf("Sending response to client (len: %zu)\n", kaam->resp_len);
                    
                    if(send_all(Gahak.client_fd, &kaam->resp_len, sizeof(size_t)) <= 0) {
                        perror("Failed to send response length");
                    } else {
                        
                        if(send_all(Gahak.client_fd, kaam->resp, kaam->resp_len) <= 0) {
                            perror("Failed to send response data");
                        } else {
                            printf("Response sent successfully\n");
                        }
                    }
                    
                    free(kaam->resp);
                } else {
                    printf("No response to send\n");
                }
            
                if(kaam->filename) {
                    free(kaam->filename);
                }
                pthread_mutex_destroy(&kaam->m);
                pthread_cond_destroy(&kaam->cv);
                free(kaam);
            } else {
                printf("Invalid command\n");
                char* err_msg = "INVALID COMMAND";
                size_t err_len = strlen(err_msg) + 1;
                send_all(Gahak.client_fd, &err_len, sizeof(size_t));
                send_all(Gahak.client_fd, err_msg, err_len);
            }
            
            free(Cmd);
        }
        
        close(Gahak.client_fd);
        printf("Client connection closed\n");
    }
    return NULL;
}

void handle_signint(int signum){
    printf("\nReceived SIGINT, shutting down...\n");
    ServerRunning = 0;
    close(server_fd);
    pthread_cond_broadcast(&ClientQueCV);
    pthread_cond_broadcast(&TaskQueCV);
}

int main(){
    if(init_db("DB/users.db")!=0){
        perror("\nCan not init the db\n");
        return 0;
    }

    int client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    pthread_t ClienThreadPool[ThreadPoolSize];
    pthread_t WorkerThreadPool[ThreadPoolSize];

    GlobalFileLockTable = file_lock_table_init();
    if (!GlobalFileLockTable) {
        fprintf(stderr, "Failed to initialize file lock table\n");
        return 1;
    }

    signal(SIGINT, handle_signint);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Failed to start the server");
        exit(0);
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(0);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Failed to bind the server\n");
        exit(0);
    }

    if(listen(server_fd, 10) < 0){
        perror("Server failed to listen");
        exit(0);
    }

    printf("Server listening on port %d...\n", server_port);

    ClntQue = queue_init(sizeof(client));
    TaskQue = queue_init(sizeof(task*));

    for(int i=0; i<ThreadPoolSize; i++){
        pthread_create(&ClienThreadPool[i], NULL, &handle_client, NULL);
    }
    
    for(int i=0; i<ThreadPoolSize; i++){
        pthread_create(&WorkerThreadPool[i], NULL, &handle_worker, NULL);
    }
    
    printf("Thread pools created (size: %d each)\n", ThreadPoolSize);

    while(ServerRunning){
        if((client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
            if(!ServerRunning)
                break;
            perror("Failed to accept client");
            continue;
        }
        
        printf("\n=== New client connected (fd: %d) ===\n", client_fd);
        
        client Gahak;
        Gahak.client_fd = client_fd;
        
        pthread_mutex_lock(&ClientQueMutuex);
        queue_enqueue(ClntQue, &Gahak);
        pthread_cond_signal(&ClientQueCV);
        pthread_mutex_unlock(&ClientQueMutuex);
    }

    printf("\nShutting down server...\n");
    close(server_fd);
    
    for(int i=0; i<ThreadPoolSize; i++){
        pthread_join(ClienThreadPool[i], NULL);
    }
    
    for(int i=0; i<ThreadPoolSize; i++){
        pthread_join(WorkerThreadPool[i], NULL);
    }

    printf("All threads joined\n");
    file_lock_table_print(GlobalFileLockTable);  
    file_lock_table_destroy(GlobalFileLockTable);
    queue_destroy(ClntQue);
    queue_destroy(TaskQue);
    printf("Server shutdown complete\n");
    
    return 0;
}
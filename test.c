#include<stdio.h>
#include<stdlib.h>
#include<string.h>

char* firstWord(char* Stmt){
    char *spacePos = strchr(Stmt, ' ');
    char *firstWord = NULL;
    if(spacePos){
        size_t len = spacePos-Stmt;
        firstWord = malloc(len+1);
        strncpy(firstWord, Stmt, len);
        firstWord[len] = '\0';
        memmove(Stmt, spacePos + 1, strlen(spacePos + 1) + 1);
    }
    else{
        firstWord = strdup(Stmt);
        Stmt[0]='\0';
    }
    return firstWord;
}

int main(){
    char *cptr = malloc(13 * sizeof(char));
    if (!cptr) {
        perror("malloc failed");
        return 1;
    }
    strcpy(cptr, "hasnain raza"); 

    char *first = firstWord(cptr);

    printf("First word: %s\n", first);
    printf("Modified original: %s\n", cptr);

    free(first);
    free(cptr);

    return 0;
}
int main_2() {

    int num = 12345;
    char buffer[20]; // Sufficient buffer size

    char * temp = itoa(num, buffer, 10); // Convert to decimal string
    printf("The address of he %p\n",(void*)temp);
    printf("The address of he %p\n",(void*)buffer);
    printf("Decimal: %s\n", buffer);

    itoa(num, buffer, 16); // Convert to hexadecimal string
    printf("Hexadecimal: %s\n", buffer);

    int neg_num = -678;
    itoa(neg_num, buffer, 10); // Convert negative decimal string
    printf("Negative Decimal: %s\n", buffer);

    return 0;
}

int main_3(){

    char *cptr = malloc(12 * sizeof(char));
    if (!cptr) {
        perror("malloc failed");
        return 1;
    }
    strcpy(cptr, "hasnain raz");
    char staticchar[15];
    strncpy(staticchar,cptr,strlen(cptr));
    free(cptr);

    printf("%c\n",staticchar);

    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "imprimer.h"
#include "helpers.h"

/*
 * "Imprimer" printer spooler.
 */
char *types[100];
int indTypes = 0;
PRINTER_SET pSet = 0;
PRINTER printers[800];
int indPrinters = 0;
int currID = 0;
int pathSize = 0;

typedef struct conversions{
    char *fileType1;
    char *fileType2;
    char *programName;
    char *arguments[50];
    int argsLength;
}CONVERSIONS;

typedef struct vertex{
    int matrixValue;
    int visited;
}VERTEX;

VERTEX typesMatrix[100][100];
CONVERSIONS conversions[100];
int conversionsInd = 0;
PRINTER chosenPrinter;
int foundPrinter = 0;
JOB jobQueue[100];
int jobInd = 0;
char *eliPrinters[100];
int eliPInd = 0;
char *inputS;

int getTypeNumber(char *fileType){
    for(int i = 0; i < indTypes; i++){
        if(strcmp(fileType, types[i]) == 0){
            return i;
        }
    }
    return -1;
}

void unvisit(){
    for(int i = 0; i < indTypes; i++){
        for(int j = 0; j < indTypes; j++){
            typesMatrix[i][j].visited = 0;
        }
    }
}

char *elimWhitespace(char *text) {
   int length, c, d;
   char *start;

   c = d = 0;

   length = strlen(text);

   start = (char*)malloc(length+1);

   if (start == NULL)
      exit(EXIT_FAILURE);

   while (*(text+c) != '\0') {
      if (*(text+c) == ' ') {
         int temp = c + 1;
         if (*(text+temp) != '\0') {
            while (*(text+temp) == ' ' && *(text+temp) != '\0') {
               if (*(text+temp) == ' ') {
                  c++;
               }
               temp++;
            }
         }
      }
      *(start+d) = *(text+c);
      c++;
      d++;
   }
   *(start+d)= '\0';

   return start;
}

char **getConversionPath(char *fileType, PRINTER printer, int* sizeAddress){
    int fileNum1 = getTypeNumber(fileType);
    int fileNum2 = getTypeNumber(printer.type);
    char **path = malloc(500);
    *path = fileType;
    path++;
    *sizeAddress = 1;
    typesMatrix[fileNum1][fileNum1].visited = 1;
    int currentRow = fileNum1;
    while(1){
        int foundNode = 0;
        int i;
        for(i = 0; i < indTypes; i++){
            if(typesMatrix[currentRow][i].matrixValue == 1 && typesMatrix[currentRow][i].visited == 0){
                foundNode = 1;
                typesMatrix[currentRow][i].visited = 1;
                break;
            }
        }

        if(foundNode){
            *path = types[i];
            path++;
            *sizeAddress = (*sizeAddress) + 1;
            if(i == fileNum2){
                path = path - (*sizeAddress);
                unvisit();
                return path;
            }
            currentRow = i;
            continue;
        }

        else{
            if(currentRow == fileNum1){
                unvisit();
                return NULL;
            }
            path--;
            *path = NULL;
            *sizeAddress = (*sizeAddress) - 1;
            currentRow = getTypeNumber(*(path-1));
        }
    }
    unvisit();
    return NULL;
}

char **getJobPath(char *backup, char *ext){
    char *word;
    word = strtok(backup, " ");
    word = strtok(NULL, " ");
    word = strtok(NULL, " ");
    if(word){
        do{
            eliPrinters[eliPInd] = word;
            eliPInd++;
        }
        while((word = strtok(NULL, " ")) != NULL);
    }

    if(eliPInd == 0){
        int foundPath = 0;
        pathSize = 0;
        char **path;
        for(int i = 0; i < indPrinters; i++){
            if(strcmp(ext, printers[i].type)== 0 && !printers[i].busy && printers[i].enabled){
                path = malloc(8);
                *path = ext;
                pathSize = 1;
                chosenPrinter = printers[i];
                foundPrinter = 1;
                break;
            }
            path = getConversionPath(ext, printers[i], &pathSize);
            if(path != NULL && !printers[i].busy && printers[i].enabled){
                foundPath = 1;
                chosenPrinter = printers[i];
                foundPrinter = 1;
                break;
            }
            free(path);
        }

        if(foundPath){
            return path;
        }
        return NULL;
    }
    else{
        int foundPath = 0;
        pathSize = 0;
        char **path;
        for(int i = 0; i < indPrinters; i++){
            if(strcmp(ext, printers[i].type)== 0 && !printers[i].busy && printers[i].enabled){
                for(int j = 0; j < eliPInd; j++){
                    if(strcmp(eliPrinters[j], printers[i].name) == 0){
                        path = malloc(8);
                        *path = ext;
                        pathSize= 1;
                        chosenPrinter = printers[i];
                        foundPrinter = 1;
                        break;
                    }
                }
                break;
            }
            path = getConversionPath(ext, printers[i], &pathSize);
            if(path != NULL && printers[i].enabled && !printers[i].busy){
                for(int j = 0; j < eliPInd; j++){
                    if(strcmp(eliPrinters[j], printers[i].name) == 0){
                        foundPath = 1;
                        chosenPrinter = printers[i];
                        foundPrinter = 1;
                        break;
                    }
                }
                if(foundPath)
                    break;
            }
            free(path);
        }

        if(foundPath){
            return path;
        }
        return NULL;
    }
}

void pipeLineHandler(int sig){
    pid_t pid;
    int status;
    pid = wait(&status);
    for(int i = 0; i < jobInd; i++){
        if(jobQueue[i].pgid == pid){
            if(status == 0){
                jobQueue[i].status = COMPLETED;
            }
            else
                jobQueue[i].status = ABORTED;

            printf("\n");
            char *jobOutput = malloc(200);
            gettimeofday(&jobQueue[i].change_time, NULL);
            imp_format_job_status(&jobQueue[i], jobOutput, 200);
            char *printerOutput = malloc(200);
            jobQueue[i].chosen_printer->busy = 0;
            imp_format_printer_status(jobQueue[i].chosen_printer, printerOutput, 200);
            printf("%s\n", jobOutput);
            printf("%s\n", printerOutput);
            checkQueuedJobs();
            break;
        }
    }
}

int changeRead(int *readPipe){
    if(close(readPipe[1]) < 0)
        return 0;
    if(dup2(readPipe[0], 0) < 0)
        return 0;
    if(close(readPipe[0]) < 0)
        return 0;
    return 1;
}

int changeWrite(int *writePipe){
    if(close(writePipe[0]) < 0){
        return 0;
    }
    if(dup2(writePipe[1], 1) < 0){
        return 0;
    }
    if(close(writePipe[1]) < 0)
        return 0;
    return 1;
}

char *backup;
char *secondBackup;

int reset(){
    foundPrinter = 0;
    free(backup);
    free(secondBackup);
    return 1;
}

void checkCompleted(){
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    for(int i = 0; i < jobInd; i++){
        if(currentTime.tv_sec - jobQueue[i].change_time.tv_sec >= 60 && (jobQueue[i].status == COMPLETED || jobQueue[i].status == ABORTED)){
            for(int j = i; j < jobInd-1; j++){
                jobQueue[j] = jobQueue[j+1];
                jobQueue[j].jobid--;
            }
            jobInd--;
        }
    }
}

void checkQueuedJobs(){
    for(int i = 0; i < jobInd; i++){
        if(jobQueue[i].status == QUEUED){
            char *ext = jobQueue[i].file_type;
            char **path;
            char *string = "print testFile";
            char *copiedString = malloc(1000);
            strcpy(copiedString, string);
            if(jobQueue[i].eligible_printers != 0xffffffff){
                for(int j = 0; j < 32; j++){
                    if(((jobQueue[i].eligible_printers >> j)&1) == 1){
                        strcat(copiedString," ");
                        for(int x = 0; x < indPrinters; x++){
                            if(printers[x].id == j){
                                strcat(copiedString, printers[x].name);
                                break;
                            }
                        }
                    }
                }
            }
            path = getJobPath(copiedString, ext);
            free(copiedString);
            char *programs[100];
            int programsInd = 0;
            char *args[100][100];
            for(int m = 0; m < 100; m++){
                for(int n = 0; n < 100; n++){
                    args[m][n] = 0;
                }
            }
            if(!foundPrinter)
                continue;
            else{
                chosenPrinter.busy = 1;
                char *out = malloc(200);
                imp_format_printer_status(&chosenPrinter, out, 200);
                printf("%s\n", out);
                free(out);
                if(pathSize == 1){
                    programs[0] = "/bin/cat";
                    programsInd++;
                }
                else{
                    for(int m = 0; m < pathSize-1; m++){
                        for(int j = 0; j < conversionsInd; j++){
                            if(strcmp(conversions[j].fileType1, *(path))==0
                                && strcmp(conversions[j].fileType2, *(path+1))==0){
                                programs[programsInd] = conversions[j].programName;
                                for(int x = 0; x < conversions[j].argsLength; x++){
                                    args[programsInd][x] = conversions[j].arguments[x];
                                }
                                programsInd++;
                                break;
                            }
                        }
                        path++;
                    }
                }
                int pId;
                char *word = jobQueue[i].file_name;
                if((pId = fork()) == 0){
                    if(programsInd == 1){
                        int childStatus;
                        sigset_t mask_child;
                        sigemptyset(&mask_child);
                        sigaddset(&mask_child, SIGCHLD);
                        sigprocmask(SIG_BLOCK, &mask_child, NULL);

                        if((pId = fork()) == 0){
                            int fd = open(word, O_RDONLY);
                            if(dup2(fd, STDIN_FILENO) < 0){
                                exit(1);
                            }

                            if (close(fd)<0){
                                exit(1);
                            }
                            int fdPrinter =imp_connect_to_printer(&chosenPrinter, 0);

                            if(dup2(fdPrinter, 1) < 0){
                                exit(1);
                            }
                            if(close(fdPrinter) < 0){
                                exit(1);
                            }

                            char *argv[100];
                            argv[0] = programs[0];
                            int w = 0;
                            while(args[0][w] != 0){
                                argv[w+1] = args[0][w];
                                w++;
                            }
                            char * const envp[] = {NULL};
                            if(execve(programs[0], argv, envp) == -1){
                                exit(1);
                            }
                            printf("\n");
                            exit(0);
                        }

                        else{
                            wait(&childStatus);
                            if (WIFEXITED(childStatus)){
                                if(WEXITSTATUS(childStatus) == 0){
                                    exit(0);
                                }
                                else
                                    exit(1);
                             }
                            else
                                 exit(1);
                        }
                    }

                    else{
                        sigset_t mask_child;
                        sigemptyset(&mask_child);
                        sigaddset(&mask_child, SIGCHLD);
                        sigprocmask(SIG_BLOCK, &mask_child, NULL);
                        int rPipe[2], wPipe[2];
                        int idSize = programsInd;

                        for(int l = 0; l < idSize; l++){
                            int childStatus;
                            pipe(wPipe);
                                if(l == 0){
                                    if((pId = fork()) == 0){
                                        int fd = open(word, O_RDONLY);
                                        if(dup2(fd, STDIN_FILENO) < 0){
                                            exit(1);
                                        }

                                        if (close(fd)<0){
                                            exit(1);
                                        }
                                        if(!changeWrite(wPipe)){

                                            exit(1);
                                        }
                                        char *argv[100];
                                        argv[0] = programs[0];
                                        int w = 0;
                                        while(args[0][w] != 0){
                                            argv[w+1] = args[0][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[0], argv, envp) == -1){
                                            exit(1);
                                        }
                                        exit(0);
                                    }
                                    waitpid(pId, &childStatus, 0);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0){
                                            exit(1);
                                        }
                                     }
                                    else
                                         exit(1);
                                    rPipe[0] = wPipe[0];
                                    rPipe[1] = wPipe[1];
                                }

                                else if(l == idSize-1){
                                    if((pId = fork()) == 0){
                                        changeRead(rPipe);
                                        close(wPipe[0]);
                                        close(wPipe[1]);
                                        int fdPrinter =imp_connect_to_printer(&chosenPrinter, 0);
                                        if(dup2(fdPrinter,1) < 0){
                                            exit(1);
                                        }
                                        if(close(fdPrinter) < 0){
                                            exit(1);
                                        }
                                        char *argv[100];
                                        argv[0] = programs[l];
                                        int w = 0;
                                        while(args[l][w] != 0){
                                            argv[w+1] = args[l][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[l], argv, envp) == -1){
                                            exit(1);
                                        }
                                        exit(0);
                                    }
                                    waitpid(-1, &childStatus, WNOHANG | WUNTRACED);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0)
                                            exit(1);
                                     }
                                    else
                                         exit(1);
                                    close(rPipe[0]);
                                    close(rPipe[1]);
                                    printf("\n");
                                    exit(0);
                                }
                                else{
                                    if((pId = fork()) == 0){
                                        if(!changeRead(rPipe))
                                            exit(1);
                                        if(!changeWrite(wPipe))
                                            exit(1);

                                        char *argv[100];
                                        argv[0] = programs[l];
                                        int w = 0;
                                        while(args[l][w] != 0){
                                            argv[w+1] = args[l][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[l], argv, envp) == -1){

                                            exit(1);
                                        }

                                        exit(0);
                                    }
                                    waitpid(-1, &childStatus, WNOHANG | WUNTRACED);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0)
                                            exit(1);
                                     }
                                    else
                                         exit(1);
                                    close(rPipe[0]);
                                    close(rPipe[1]);
                                    rPipe[0] = wPipe[0];
                                    rPipe[1] = wPipe[1];
                                }
                        }
                    }
                }

                else{
                    printf("\n");
                    setpgid(getpid(), getpid());
                    jobQueue[i].status = RUNNING;
                    jobQueue[i].pgid = pId;
                    jobQueue[i].chosen_printer = &chosenPrinter;
                    gettimeofday(&jobQueue[i].change_time, NULL);
                    char *jobStat =  malloc(400);
                    imp_format_job_status(&jobQueue[i], jobStat, 400);
                    printf("%s\n", jobStat);
                    free(jobStat);
                }
            }
            eliPInd = 0;
            if(path != NULL){
                path = path - (pathSize-1);
                free(path);
            }
        }
    }
}

int runImprimer(char *input){
    input = elimWhitespace(input);
    checkCompleted();
    backup = malloc(strlen(input) + 1);
    strcpy(backup, input);
    secondBackup = malloc(strlen(input) + 1);
    strcpy(secondBackup, input);
    char *word = strtok(input, " ");
    int done = 0;
    while(!done){
        if(strcmp(word, "type") == 0){
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            types[indTypes] = word;
            indTypes++;
            printf("New type added successfully\n");
        }

        else if(strcmp(word, "printer") == 0){
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            printers[indPrinters].id = currID;
            printers[indPrinters].name = word;
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            int isType = 0;
            for(int i = 0; i < indTypes; i++){
                if(strcmp(word, types[i]) == 0){
                    isType = 1;
                }
            }
            if(isType == 0){
                char *err = malloc(200);
                imp_format_error_message("Invalid Type", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            printers[indPrinters].type = word;
            printers[indPrinters].enabled = 0;
            printers[indPrinters].busy = 0;
            pSet |= 1 << currID;
            currID++;
            indPrinters++;
            char *pFormat = malloc(200);
            PRINTER *pPointer = &printers[indPrinters-1];
            imp_format_printer_status(pPointer, pFormat, 200);
            printf("%s\n", pFormat);
            free(pFormat);
        }

        else if(strcmp(word, "printers") == 0){
            for(int i = 0; i < indPrinters; i++){
                PRINTER *pPoint;
                pPoint = &printers[i];
                char *buf = malloc(200);
                imp_format_printer_status(pPoint, buf, 200);
                printf("%s\n", buf);
                free(buf);
            }
        }

        else if(strcmp(word, "jobs") == 0){
            for(int i = 0; i < jobInd; i++){
                char *output = malloc(200);
                imp_format_job_status(&jobQueue[i], output, 200);
                printf("%s\n", output);
                free(output);
            }
        }

        else if(strcmp(word, "disable") == 0){
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            for(int i = 0; i < indPrinters; i++){
                if(strcmp(printers[i].name, word) == 0){
                    if(printers[i].enabled){
                        printers[i].enabled = 0;
                        char *buf = malloc(200);
                        PRINTER *pPointer;
                        pPointer = &printers[i];
                        imp_format_printer_status(pPointer, buf, 200);
                        printf("%s\n", buf);
                        free(buf);
                        break;
                    }
                }
            }
        }

        else if(strcmp(word, "enable") == 0){
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            for(int i = 0; i < indPrinters; i++){
                if(strcmp(printers[i].name, word) == 0){
                    if(!printers[i].enabled){
                        printers[i].enabled = 1;
                        char *buf = malloc(200);
                        PRINTER *pPointer;
                        pPointer = &printers[i];
                        imp_format_printer_status(pPointer, buf, 200);
                        printf("%s\n", buf);
                        free(buf);
                        checkQueuedJobs();
                        break;
                    }
                }
            }
        }

        else if(strcmp(word, "conversion") == 0){
            int fileNum1;
            int fileNum2;
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            if((fileNum1 = getTypeNumber(word)) == -1){
                char *err = malloc(200);
                imp_format_error_message("File Type Does Not Exist", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            if((fileNum2 = getTypeNumber(word)) == -1){
                char *err = malloc(200);
                imp_format_error_message("File Type Does Not Exist", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            conversions[conversionsInd].fileType1 = types[fileNum1];
            conversions[conversionsInd].fileType2 = types[fileNum2];
            conversions[conversionsInd].programName = word;
            int i = 0;
            conversions[conversionsInd].argsLength = 0;
            word = strtok(NULL, " ");
            if(word){
                do{
                    conversions[conversionsInd].arguments[i] = word;
                    conversions[conversionsInd].argsLength++;
                    i++;
                }
                while((word = strtok(NULL, " ")) != NULL);
            }
            conversionsInd++;
            typesMatrix[fileNum1][fileNum2].matrixValue = 1;
            typesMatrix[fileNum1][fileNum2].visited = 0;
            printf("%s\n", "Successfully added conversion program");
            break;
        }

        else if(strcmp(word, "print") == 0){
            word = strtok(NULL, " ");
            if(word == NULL){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            char *fileName;
            fileName = word;
            char *ext = strtok(fileName, ".");
            ext = strtok(NULL, " ");
            if(getTypeNumber(ext) == -1){
                char *err = malloc(200);
                imp_format_error_message("Invalid File Extension", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            char **path;
            path = getJobPath(backup, ext);
            char *programs[100];
            char *args[100][100];
            for(int m = 0; m < 100; m++){
                for(int n = 0; n < 100; n++){
                    args[m][n] = 0;
                }
            }
            int programsInd = 0;
            if(!foundPrinter){
                jobQueue[jobInd].jobid = jobInd;
                jobQueue[jobInd].status = QUEUED;
                jobQueue[jobInd].pgid = 0;
                word = strtok(secondBackup, " ");
                word = strtok(NULL, " ");
                char *name = malloc(200);
                strcpy(name, word);
                jobQueue[jobInd].file_name = name;
                jobQueue[jobInd].file_type = ext;
                PRINTER_SET eligible = 0;;
                if(eliPInd == 0){
                    eligible = 0xffffffff;
                    jobQueue[jobInd].eligible_printers = eligible;
                }
                else{
                    for(int i = 0; i < indPrinters; i++){
                        for(int j = 0; j < eliPInd; j++){
                            if(strcmp(printers[i].name, eliPrinters[j]) == 0){
                                eligible |= 1 << i;
                                break;
                            }
                        }
                    }
                    jobQueue[jobInd].eligible_printers = eligible;
                }
                gettimeofday(&jobQueue[jobInd].creation_time, NULL);
                gettimeofday(&jobQueue[jobInd].change_time, NULL);
                char *jobStat =  malloc(400);
                imp_format_job_status(&jobQueue[jobInd], jobStat, 400);
                printf("%s\n", jobStat);
                jobInd++;
                free(jobStat);

            }


            else{
                chosenPrinter.busy = 1;
                char *out = malloc(200);
                imp_format_printer_status(&chosenPrinter, out, 200);
                printf("%s\n", out);
                free(out);
                if(pathSize == 1){
                    programs[0] = "/bin/cat";
                    programsInd++;
                }
                else{
                    for(int i = 0; i < pathSize-1; i++){
                        for(int j = 0; j < conversionsInd; j++){
                            if(strcmp(conversions[j].fileType1, *(path))==0
                                && strcmp(conversions[j].fileType2, *(path+1))==0){
                                programs[programsInd] = conversions[j].programName;
                                for(int x = 0; x < conversions[j].argsLength; x++){
                                    args[programsInd][x] = conversions[j].arguments[x];
                                }
                                programsInd++;
                                break;
                            }
                        }
                        path++;
                    }
                }
                int pId;
                word = strtok(secondBackup, " ");
                word = strtok(NULL, " ");
                if((pId = fork()) == 0){
                    if(programsInd == 1){
                        int childStatus;
                        sigset_t mask_child;
                        sigemptyset(&mask_child);
                        sigaddset(&mask_child, SIGCHLD);
                        sigprocmask(SIG_BLOCK, &mask_child, NULL);

                        if((pId = fork()) == 0){

                            int fd = open(word, O_RDONLY);
                            if(dup2(fd, STDIN_FILENO) < 0){
                                exit(1);
                            }

                            if (close(fd)<0){
                                exit(1);
                            }
                            int fdPrinter =imp_connect_to_printer(&chosenPrinter, 0);

                            if(dup2(fdPrinter, 1) < 0){
                                exit(1);
                            }
                            if(close(fdPrinter) < 0){
                                exit(1);
                            }

                            char *argv[100];
                            argv[0] = programs[0];
                            int w = 0;
                            while(args[0][w] != 0){
                                argv[w+1] = args[0][w];
                                w++;
                            }
                            char * const envp[] = {NULL};
                            if(execve(programs[0], argv, envp) == -1){
                                exit(1);
                            }
                            printf("\n");
                            exit(0);
                        }

                        else{
                            wait(&childStatus);
                            if (WIFEXITED(childStatus)){
                                if(WEXITSTATUS(childStatus) == 0)
                                    exit(0);
                                else
                                    exit(1);
                             }
                            else
                                 exit(1);
                        }
                    }

                    else{
                        sigset_t mask_child;
                        sigemptyset(&mask_child);
                        sigaddset(&mask_child, SIGCHLD);
                        sigprocmask(SIG_BLOCK, &mask_child, NULL);
                        int rPipe[2], wPipe[2];
                        int idSize = programsInd;

                        for(int i = 0; i < idSize; i++){
                            int childStatus;
                            pipe(wPipe);
                                if(i == 0){
                                    if((pId = fork()) == 0){
                                        int fd = open(word, O_RDONLY);
                                        if(dup2(fd, STDIN_FILENO) < 0){
                                            exit(1);
                                        }

                                        if (close(fd)<0){
                                            exit(1);
                                        }
                                        if(!changeWrite(wPipe)){

                                            exit(1);
                                        }
                                        char *argv[100];
                                        argv[0] = programs[0];
                                        int w = 0;
                                        while(args[0][w] != 0){
                                            argv[w+1] = args[0][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[0], argv, envp) == -1){
                                            exit(1);
                                        }
                                        exit(0);
                                    }
                                    waitpid(pId, &childStatus, 0);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0){
                                            exit(1);
                                        }
                                     }
                                    else
                                         exit(1);
                                    rPipe[0] = wPipe[0];
                                    rPipe[1] = wPipe[1];
                                }

                                else if(i == idSize-1){
                                    if((pId = fork()) == 0){
                                        changeRead(rPipe);
                                        close(rPipe[0]);
                                        close(wPipe[0]);
                                        close(wPipe[1]);
                                        int fdPrinter =imp_connect_to_printer(&chosenPrinter, 0);
                                        if(dup2(fdPrinter,1) < 0){
                                            exit(1);
                                        }
                                        if(close(fdPrinter) < 0){
                                            exit(1);
                                        }
                                        char *argv[100];
                                        argv[0] = programs[i];
                                        int w = 0;
                                        while(args[i][w] != 0){
                                            argv[w+1] = args[i][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[i], argv, envp) == -1){
                                            exit(1);
                                        }
                                        exit(0);
                                    }
                                    waitpid(-1, &childStatus, WUNTRACED | WNOHANG);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0)
                                            exit(1);
                                     }
                                    else
                                         exit(1);
                                    close(rPipe[0]);
                                    close(rPipe[1]);
                                    printf("\n");
                                    exit(0);
                                }
                                else{
                                    if((pId = fork()) == 0){
                                        if(!changeRead(rPipe))
                                            exit(1);
                                        if(!changeWrite(wPipe))
                                            exit(1);

                                        char *argv[100];
                                        argv[0] = programs[i];
                                        int w = 0;
                                        while(args[i][w] != 0){
                                            argv[w+1] = args[i][w];
                                            w++;
                                        }
                                        char * const envp[] = {NULL};
                                        if(execve(programs[i], argv, envp) == -1){

                                            exit(1);
                                        }

                                        exit(0);
                                    }
                                    waitpid(-1, &childStatus, WUNTRACED | WNOHANG);
                                    if (WIFEXITED(childStatus)){
                                        if(WEXITSTATUS(childStatus) != 0)
                                            exit(1);
                                     }
                                    else
                                         exit(1);
                                    close(rPipe[0]);
                                    close(rPipe[1]);
                                    rPipe[0] = wPipe[0];
                                    rPipe[1] = wPipe[1];
                                }
                        }
                    }
                }

                else{
                    printf("\n");
                    setpgid(getpid(), getpid());
                    jobQueue[jobInd].jobid = jobInd;
                    jobQueue[jobInd].status = RUNNING;
                    jobQueue[jobInd].pgid = pId;
                    jobQueue[jobInd].file_name = word;
                    jobQueue[jobInd].file_type = ext;
                    jobQueue[jobInd].chosen_printer = &chosenPrinter;
                    PRINTER_SET eligible = 0;;
                    if(eliPInd == 0){
                        eligible = 0xffffffff;
                        jobQueue[jobInd].eligible_printers = eligible;
                    }
                    else{
                        for(int i = 0; i < indPrinters; i++){
                            for(int j = 0; j < eliPInd; j++){
                                if(strcmp(printers[i].name, eliPrinters[j]) == 0){
                                    eligible |= 1 << i;
                                    break;
                                }
                            }
                        }
                        jobQueue[jobInd].eligible_printers = eligible;
                    }
                    gettimeofday(&jobQueue[jobInd].creation_time, NULL);
                    gettimeofday(&jobQueue[jobInd].change_time, NULL);
                    char *jobStat =  malloc(200);
                    imp_format_job_status(&jobQueue[jobInd], jobStat, 200);
                    printf("%s\n", jobStat);
                    jobInd++;
                    free(jobStat);
                }
            }
            eliPInd = 0;
            if(path != NULL){
                path = path - (pathSize-1);
                free(path);
            }
        }

        else if(strcmp(word, "pause") == 0){
            word = strtok(NULL, " ");
            if(word == NULL || atoi(word) >= jobInd || jobQueue[atoi(word)].status != RUNNING){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            kill(jobQueue[atoi(word)].pgid, SIGSTOP);
            jobQueue[atoi(word)].status = PAUSED;
            char *jobStat =  malloc(400);
            imp_format_job_status(&jobQueue[atoi(word)], jobStat, 400);
            printf("%s\n", jobStat);
            free(jobStat);
        }

        if(strcmp(word, "resume") == 0){
            word = strtok(NULL, " ");
            if(word == NULL || atoi(word) >= jobInd || jobQueue[atoi(word)].status != PAUSED){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            kill(jobQueue[atoi(word)].pgid, SIGCONT);
            jobQueue[atoi(word)].status = RUNNING;
            char *jobStat =  malloc(400);
            imp_format_job_status(&jobQueue[atoi(word)], jobStat, 400);
            printf("%s\n", jobStat);
            free(jobStat);
        }

        if(strcmp(word, "help") == 0){
            printf("MISCELLANEOUS COMMANDS:\n");
            printf("help - display help menu\nquit - exit the program\n\n");
            printf("CONFIGURATION COMMANDS:\n");
            printf("type (file_type) - Create new file type\n");
            printf("printer (printer_name file_type) - Declare printer of type file_type\n");
            printf("conversion (file_type1 file_type2 conversion_program [arg1 arg2...]) - Declare new conversion program\n\n");
            printf("INFORMATIONAL COMMANDS:\n");
            printf("printers - Display all declared printers\n");
            printf("jobs - Display all current jobs\n\n");
            printf("SPOOLING COMMANDS:\n");
            printf("print (file_name [printer1 printer2...]) - Print file file_name, and specify optional set of eligible printers\n");
            printf("cancel (job_number) - Cancel a current jobs\n");
            printf("pause (job_number) - Pause a current job\n");
            printf("resume (job_number) - Resume a currently paused job\n");
            printf("disable (printer_name) - Disable printer printer_name\n");
            printf("enable (printer_name) - Enable printer printer_name\n");
        }

        else if(strcmp(word, "cancel") == 0){
            word = strtok(NULL, " ");
            if(word == NULL || atoi(word) >= jobInd){
                char *err = malloc(200);
                imp_format_error_message("Invalid Command", err, 200);
                printf("%s\n", err);
                free(err);
                break;
            }
            if(jobQueue[atoi(word)].status == RUNNING)
                kill(jobQueue[atoi(word)].pgid, SIGTERM);
            for(int i = atoi(word); i < jobInd-1; i++){
                jobQueue[i] = jobQueue[i+1];
                jobQueue[i].jobid--;
            }
            jobInd--;

        }

        else if(strcmp(word, "quit") == 0){
            exit(EXIT_SUCCESS);
        }

        done = 1;
    }
    return reset();
}

int main(int argc, char *argv[])
{
    signal(SIGCHLD, pipeLineHandler);
    char *output;
    int directToFile = 0;
    if(argc > 1 && strcmp(argv[1], "-i") == 0){
        if(argc == 5 && strcmp(argv[3], "-o") == 0){
            output = argv[4];
            freopen(output, "a+", stdout);
            directToFile = 1;
        }
        if(argc < 3)
            exit(1);
        FILE *fp;
        char *buffer = malloc(300);
        if ((fp = fopen(argv[2], "r")) == NULL)
        {
            exit(1);
        }

        while (fgets(buffer, 300, fp) != NULL)
        {
            buffer[strlen(buffer) - 1] = '\0';
            for(int i = 0; i < strlen(buffer); i++){
                if(buffer[i] == '\r'){
                    buffer[i] = '\0';
                    break;
                }
            }
            char *s = malloc(300);
            memcpy(s, buffer, 300);
            runImprimer(s);
        }
        fclose(fp);
    }

    else if(argc > 4 && strcmp(argv[3], "-i") == 0){

        if(argc == 5 && strcmp(argv[1], "-o") == 0){
            output = argv[2];
            freopen(output, "a+", stdout);
            directToFile = 1;
        }

        if(argc < 3)
            exit(1);
        FILE *fp;
        char *buffer = malloc(300);
        if ((fp = fopen(argv[4], "r")) == NULL)
        {
            exit(1);
        }

        while (fgets(buffer, 300, fp) != NULL)
        {
            buffer[strlen(buffer) - 1] = '\0';
            for(int i = 0; i < strlen(buffer); i++){
                if(buffer[i] == '\r'){
                    buffer[i] = '\0';
                    break;
                }
            }
            char *s = malloc(300);
            memcpy(s, buffer, 300);
            runImprimer(s);
        }
        fclose(fp);
    }
    if(directToFile)
        freopen("/dev/tty", "w", stdout);
    while(strcmp((inputS = readline("imp>")),"quit") != 0){
        if(directToFile)
            freopen(output, "a+", stdout);
        runImprimer(inputS);
        if(directToFile)
            freopen("/dev/tty", "w", stdout);
    }
    exit(EXIT_SUCCESS);
}

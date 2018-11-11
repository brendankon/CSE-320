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
char *input;

int getTypeNumber(char *fileType){
    for(int i = 0; i < indTypes; i++){
        if(strcmp(types[i], fileType) == 0){
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
            if(strcmp(ext, printers[i].type)== 0 && !printers[i].busy){
                path = malloc(8);
                *path = ext;
                pathSize++;
                chosenPrinter = printers[i];
                foundPrinter = 1;
                break;
            }
            path = getConversionPath(ext, printers[i], &pathSize);
            if(path != NULL && !printers[i].busy){
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
            if(strcmp(ext, printers[i].type)== 0){
                for(int j = 0; j < eliPInd; j++){
                    if(strcmp(eliPrinters[j], printers[i].name) == 0){
                        path = malloc(8);
                        *path = ext;
                        pathSize++;
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
            imp_format_job_status(&jobQueue[i], jobOutput, 200);
            char *printerOutput = malloc(200);
            jobQueue[i].chosen_printer->busy = 0;
            imp_format_printer_status(jobQueue[i].chosen_printer, printerOutput, 200);
            printf("%s\n", jobOutput);
            printf("%s\n", printerOutput);
            break;
        }
    }
    printf("imp>");
}

int main(int argc, char *argv[])
{
    char *backup;
    char *secondBackup;
    signal(SIGCHLD, pipeLineHandler);

    while(strcmp((input = readline("imp>")),"quit") != 0){
        backup = malloc(strlen(input));
        strcpy(backup, input);
        secondBackup = malloc(strlen(input));
        strcpy(secondBackup, input);
        char *word = strtok(input, " ");
        if(strcmp(word, "type") == 0){
            word = strtok(NULL, " ");
            types[indTypes] = word;
            indTypes++;
            printf("New type added successfully\n");
        }

        if(strcmp(word, "printer") == 0){
            word = strtok(NULL, " ");
            printers[indPrinters].id = currID;
            printers[indPrinters].name = word;
            word = strtok(NULL, " ");
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
                currID--;
                continue;
            }
            printers[indPrinters].type = word;
            printers[indPrinters].enabled = 1;
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

        if(strcmp(word, "printers") == 0){
            for(int i = 0; i < indPrinters; i++){
                PRINTER *pPoint;
                pPoint = &printers[i];
                char *buf = malloc(200);
                imp_format_printer_status(pPoint, buf, 200);
                printf("%s\n", buf);
                free(buf);
            }
        }

        if(strcmp(word, "jobs") == 0){
            for(int i = 0; i < jobInd; i++){
                char *output = malloc(200);
                imp_format_job_status(&jobQueue[i], output, 200);
                printf("%s\n", output);
                free(output);
            }
        }

        if(strcmp(word, "disable") == 0){
            word = strtok(NULL, " ");
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

        if(strcmp(word, "enable") == 0){
            word = strtok(NULL, " ");
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
                        break;
                    }
                }
            }
        }

        if(strcmp(word, "conversion") == 0){
            int fileNum1;
            int fileNum2;
            word = strtok(NULL, " ");
            if((fileNum1 = getTypeNumber(word)) == -1){
                char *err = malloc(200);
                imp_format_error_message("File Type Does Not Exit", err, 200);
                printf("%s\n", err);
                free(err);
                continue;
            }
            word = strtok(NULL, " ");
            if((fileNum2 = getTypeNumber(word)) == -1){
                char *err = malloc(200);
                imp_format_error_message("File Type Does Not Exist", err, 200);
                printf("%s\n", err);
                free(err);
                continue;
            }
            word = strtok(NULL, " ");
            conversions[conversionsInd].fileType1 = types[fileNum1];
            conversions[conversionsInd].fileType2 = types[fileNum2];
            conversions[conversionsInd].programName = word;
            conversionsInd++;
            typesMatrix[fileNum1][fileNum2].matrixValue = 1;
            typesMatrix[fileNum1][fileNum2].visited = 0;
            printf("%s\n", "Successfully added conversion program");
        }

        if(strcmp(word, "print") == 0){
            word = strtok(NULL, " ");
            char *fileName;
            fileName = word;
            char *ext = strtok(fileName, ".");
            ext = strtok(NULL, " ");
            if(getTypeNumber(ext) == -1){
                char *err = malloc(200);
                imp_format_error_message("Invalid File Extension", err, 200);
                printf("%s\n", err);
                free(err);
                continue;
            }
            char **path;
            path = getJobPath(backup, ext);
            char *programs[100];
            int programsInd = 0;
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
                            programsInd++;
                            break;
                        }
                    }
                    path++;
                }
            }

            if(!foundPrinter){
                jobQueue[jobInd].jobid = jobInd;
                jobQueue[jobInd].status = QUEUED;
                jobQueue[jobInd].pgid = 0;
                word = strtok(secondBackup, " ");
                word = strtok(NULL, " ");
                jobQueue[jobInd].file_name = word;
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
                char *jobStat =  malloc(200);
                imp_format_job_status(&jobQueue[jobInd], jobStat, 200);
                printf("%s\n", jobStat);
                jobInd++;
                free(jobStat);

            }

            else{
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
                            char * const argv[] = {"/bin/cat", NULL};
                            char * const envp[] = {NULL};
                            if(execve("/bin/cat", argv, envp) == -1){
                                exit(1);
                            }
                            printf("\n");
                            int fdPrinter =imp_connect_to_printer(&chosenPrinter, 0);
                            if(dup2(fdPrinter, 1) < 0){
                                exit(1);
                            }
                            if(close(fdPrinter) < 0){
                                exit(1);
                            }
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
                    continue;
                }
            }
            printf("%s\n", programs[0]);
            eliPInd = 0;
            path = path - (pathSize-1);
            free(path);
        }
    }
    free(input);
    free(backup);
    free(secondBackup);
    exit(EXIT_SUCCESS);
}

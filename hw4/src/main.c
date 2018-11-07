#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

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

int main(int argc, char *argv[])
{
    char *input;
    while(strcmp((input = readline("imp>")),"quit") != 0){
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
                printf("Error, invalid type\n");
                currID--;
                continue;
            }
            printers[indPrinters].type = word;
            printers[indPrinters].enabled = 1;
            printers[indPrinters].busy = 0;
            pSet |= 1 << currID;
            currID++;
            indPrinters++;
            printf("New printer added successfully\n");
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
    }
    free(input);
    exit(EXIT_SUCCESS);
}

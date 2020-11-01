#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <myconfig.h>
#include "siparse.h"
#include "config.h"
#include "builtins.h"

int argseqLength(argseq* args){
    argseq* temp = args;
    int count = 0;
    do{
        count++;
        temp = temp->next;
    }while(temp!=args);
    char* tab[count];
    return count;
}

void writeErrorForProgram(char* name, char* content){
    unsigned long long size = strlen(name)+strlen(content)+3;
    char s[size];
    snprintf(s, size, "%s: %s", name, content);
    write(STDERR, s, sizeof(s)-1);
}

void writeSyntaxError(){
    unsigned long long size = strlen(SYNTAX_ERROR_STR)+2;
    char s[size];
    snprintf(s, size, "%s%c", SYNTAX_ERROR_STR, '\n');
    write(STDERR, s, sizeof(s)-1);
}

int findEndLine(const char* buf, int begin, int end){
    for (int i=begin; i<end; i++){
        if (buf[i]=='\n')
            return i;
    }
    return -1;
}

void mystrcpy(char* dest, const char* src){
    for (int i=0;;i++){
        dest[i]=src[i];
        if (src[i]==0)
            return;
    }
}

void argsTab(command* com, char** tab){
    int i = 0;

    argseq *temp = com->args;
    do {
        tab[i] = temp->arg;
        i++;
        temp = temp->next;
    } while (temp != com->args);
    tab[i] = NULL;
}

builtin_pair* findInBuiltins(char* com){
    for(int i=0; builtins_table[i].name != NULL; i++){
        if (strcmp(builtins_table[i].name, com) == 0)
            return builtins_table + i;
    }
    return NULL;
}

char isNumber(const char* str){
    for(int i=0; str + i !=NULL; i++){
        if (str[i]<'0' || str[i]>'9')
            return 0;
    }
    return 1;
}

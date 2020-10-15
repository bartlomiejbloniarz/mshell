#ifndef SHELL_MYUTILS_H
#define SHELL_MYUTILS_H

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
    char s[strlen(name)+strlen(content)+3];
    snprintf(s, strlen(name)+strlen(content)+3, "%s: %s", name, content);
    write(STDERR, s, sizeof(s));
}

#endif //SHELL_MYUTILS_H

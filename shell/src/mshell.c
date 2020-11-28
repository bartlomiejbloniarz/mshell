#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "myconfig.h"
#include "myutils.h"

struct pair{
    int status;
    pid_t pid;
};

volatile int children=0, counter=0, pidAmount=0;
struct pair terminated[BACKGROUND_MEMORY];
pid_t pids[MAX_LINE_LENGTH];

void handler(int sig_nb){
    pid_t child;
    char check = 1;
    int wStatus;
    do{
        child = waitpid(-1, &wStatus, WNOHANG);
        if (child>0) {
            for (int i=0; i<pidAmount; i++){
                if (pids[i] == child) {
                    check = 0;
                    break;
                }
            }
            if (check) {
                terminated[counter].status = wStatus;
                terminated[counter++].pid = child;
            }
            else
                children--;
        }
    } while (child>0);
}

int managePipeLine(pipelineseq* pls, commandseq* commands, char background, sigset_t* sigintSet, sigset_t* sigChildSet){

    int fd1[2], fd2[2];
    char* input = NULL, *output = NULL;
    int inputFd, outputFd;
    pid_t pid;
    builtin_pair *builtinPair;
    char append = 0, endOfPipeline;
    command *com;

    fd1[STDIN] = -1;
    int idx = 0;

    do {
        endOfPipeline = (char)(commands->next == pls->pipeline->commands);
        com = commands->com;
        commands = commands ->next;
        if (com == NULL)
            continue;

        builtinPair = findInBuiltins(com->args->arg);

        if (builtinPair != NULL) {
            char *tab[argseqLength(com->args) + 1];
            argsTab(com, tab);
            if (builtinPair->fun(tab)) {
                int size = strlen(builtinPair->name);
                char temp[size + 17];
                strcpy(temp, "Builtin ");
                strcpy(temp + 8, builtinPair->name);
                strcpy(temp + 8 + size, " error.\n");
                write(STDERR, temp, size + 16);
            }
        } else {
            if (!endOfPipeline)
                pipe(fd2);
            pid = fork();
            if (pid == -1) {
                return EXEC_FAILURE;
            }
            if (pid == 0) {
                if (background)
                    setsid();
                sigprocmask(SIG_UNBLOCK, sigintSet, NULL);
                if (!endOfPipeline)
                    close(fd2[STDIN]);
                redirseq *redirects = com->redirs;
                if (redirects) {
                    do {
                        if (IS_RIN(redirects->r->flags))
                            input = redirects->r->filename;
                        else {
                            output = redirects->r->filename;
                            append = IS_RAPPEND(redirects->r->flags);
                        }
                        redirects = redirects->next;
                    } while (redirects != com->redirs);
                }

                if (input) {
                    inputFd = open(input, O_RDONLY);
                    if (inputFd == -1){
                        printErrors(input);
                        return EXEC_FAILURE;
                    }
                    if (dup2(inputFd, STDIN) == -1)
                        return EXEC_FAILURE;
                    close(inputFd);
                    if (fd1[STDIN] != -1)
                        close(fd1[STDIN]);
                }
                else if (fd1[STDIN] != -1){
                    if (dup2(fd1[STDIN], STDIN) == -1)
                        return EXEC_FAILURE;
                    close(fd1[STDIN]);
                }

                if (output) {
                    if (append)
                        outputFd = open(output, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
                    else
                        outputFd = open(output, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
                    if (outputFd == -1){
                        printErrors(output);
                        return EXEC_FAILURE;
                    }
                    if (dup2(outputFd, STDOUT) == -1)
                        return EXEC_FAILURE;
                    close(outputFd);
                    if (!endOfPipeline)
                        close(fd2[STDOUT]);
                }
                else if (!endOfPipeline){
                    if(dup2(fd2[STDOUT], STDOUT) == -1)
                        return EXEC_FAILURE;
                    close(fd2[STDOUT]);
                }

                char *tab[argseqLength(com->args) + 1];
                argsTab(com, tab);
                execvp(com->args->arg, tab);
                printErrors(com->args->arg);

                return EXEC_FAILURE;
            }
            else {
                if (!background) {
                    sigprocmask(SIG_BLOCK, sigChildSet, NULL);
                    if (idx < BACKGROUND_MEMORY)
                        pids[idx++] = pid;
                    sigprocmask(SIG_UNBLOCK, sigChildSet, NULL);
                }
                if (fd1[STDIN] != -1)
                    close(fd1[STDIN]);
                if (!endOfPipeline)
                    close(fd2[STDOUT]);
                else {
                    if (!background) {
                        sigset_t old;
                        sigprocmask(SIG_BLOCK, sigChildSet, &old);
                        while (children>0){
                            sigsuspend(&old);
                        }
                        pidAmount = 0;
                        sigprocmask(SIG_UNBLOCK, sigChildSet, NULL);
                    }
                }
                swap(fd1, fd2);
            }
        }
    } while(commands!=pls->pipeline->commands);

    return EXIT_SUCCESS;
}

int managePipeLineSeq(pipelineseq *ln, sigset_t* sigintSet, sigset_t* sigChildSet){

    char background;
    pipelineseq * pls = ln;
    commandseq *commands;

    do {
        commands = pls->pipeline->commands;
        int commandsLength = commandseqLength(commands);
        if (commandsLength == -1){
            writeSyntaxError();
            return 0;
        }
        background = (char)(pls->pipeline->flags == INBACKGROUND);
        if (!background) {
            sigprocmask(SIG_BLOCK, sigChildSet, NULL);
            children = commandsLength;
            pidAmount = commandsLength;
            sigprocmask(SIG_UNBLOCK, sigChildSet, NULL);
        }

        int pipeLineResult = managePipeLine(pls, commands, background, sigintSet, sigChildSet);
        if (pipeLineResult)
            return pipeLineResult;

        pls = pls->next;
    } while(pls != ln);

    return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
    pipelineseq * ln;
    int readSize, offset = 0, start = 0, stop;
    char syntaxError = 0;
    struct stat stat;
    fstat(STDIN, &stat);

    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);

    sigset_t sigintSet;
    sigemptyset(&sigintSet);
    sigaddset(&sigintSet, SIGINT);

    sigset_t sigChildSet;
    sigemptyset(&sigChildSet);
    sigaddset(&sigChildSet, SIGCHLD);

    sigprocmask(SIG_BLOCK, &sigintSet, NULL);

    char buf[2*MAX_LINE_LENGTH+2];

    if (S_ISCHR(stat.st_mode))
        write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));
    while ((readSize = read(STDIN, buf + offset, MAX_LINE_LENGTH+1))){

        if (readSize==-1)
            return EXIT_FAILURE;

        readSize += offset;
        while((stop = findEndLine(buf, start, readSize))!=-1) {

            if (syntaxError){
                syntaxError = 0;
                writeSyntaxError();
                start = stop + 1;
                continue;
            }

            ln = NULL;
            buf[stop] = 0;

            if (stop - start <= MAX_LINE_LENGTH) {
                ln = parseline(buf + start);
            }
            start = stop + 1;

            if (ln == NULL) {
                writeSyntaxError();
                continue;
            }

            int pipeLineResult = managePipeLineSeq(ln, &sigintSet, &sigChildSet);
            if (pipeLineResult)
                return pipeLineResult;

        }
        offset = readSize;
        if (S_ISCHR(stat.st_mode)) {
            sigprocmask(SIG_BLOCK, &sigChildSet, NULL);
            for (int i=0; i<counter; i++){
                writeTermOrKill(terminated[i].pid, terminated[i].status);
            }
            counter = 0;
            sigprocmask(SIG_UNBLOCK, &sigChildSet, NULL);
            write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));
        }

        if (offset>MAX_LINE_LENGTH){
            if (offset - start>MAX_LINE_LENGTH){
                offset = 0;
                syntaxError = 1;
            }
            else {
                buf[offset] = 0;
                strShift(buf, start);
                offset -= start;
            }
            start = 0;
        }

    }
    return EXIT_SUCCESS;
}
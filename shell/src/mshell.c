#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "myconfig.h"
#include "myutils.h"

int managePipeLine(pipelineseq *ln){

    pipelineseq * pls;
    pls = ln;
    command *com;
    commandseq *commands;
    int fd1[2], fd2[2];
    char* input = NULL, *output = NULL;
    int inputFd, outputFd, wStatus;
    pid_t pid;
    builtin_pair *builtinPair;
    char append = 0, endOfPipeline;

    do {
        commands = pls->pipeline->commands;
        fd1[STDIN] = -1;
        int commandsLength = commandseqLength(commands);
        if (commandsLength == -1){
            writeSyntaxError();
            pls = pls->next;
            return 0;
        }
        pid_t pids[commandsLength];
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
                    mystrcpy(temp, "Builtin ");
                    mystrcpy(temp + 8, builtinPair->name);
                    mystrcpy(temp + 8 + size, " error.\n");
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
                    pids[idx++] = pid;
                    if (fd1[STDIN] != -1)
                        close(fd1[STDIN]);
                    if (!endOfPipeline)
                        close(fd2[STDOUT]);
                    else {
                        for (int i = 0; i < commandsLength; i++)
                            waitpid(pids[i], &wStatus, 0);
                    }
                    swap(fd1, fd2);
                }
            }
        } while(commands!=pls->pipeline->commands);
        pls = pls->next;
    } while(pls != ln);
    return 0;
}

int
main(int argc, char *argv[])
{
    pipelineseq * ln;
    int readSize, offset = 0, start = 0, stop;
    char syntaxError = 0;
    struct stat stat;
    fstat(STDIN, &stat);

    char buf[2*MAX_LINE_LENGTH+2];

    if (S_ISCHR(stat.st_mode))
        write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));
    while ((readSize = read(STDIN, buf + offset, MAX_LINE_LENGTH+1))){

        if (readSize==-1)
            return 1;

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

            int pipeLineResult = managePipeLine(ln);
            if (pipeLineResult)
                return pipeLineResult;

        }
        offset = readSize;
        if (S_ISCHR(stat.st_mode))
            write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));

        if (offset>MAX_LINE_LENGTH){
            if (offset - start>MAX_LINE_LENGTH){
                offset = 0;
                syntaxError = 1;
            }
            else {
                buf[offset] = 0;
                mystrcpy(buf, buf + start);
                offset -= start;
            }
            start = 0;
        }

    }
    return 0;
}

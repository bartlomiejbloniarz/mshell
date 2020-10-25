#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "myconfig.h"
#include "myutils.h"

int
main(int argc, char *argv[])
{
	pipelineseq * ln;
	command *com;
	int readSize, wStatus, offset = 0, start = 0, stop;
	char syntaxError = 0;
	pid_t pid;
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

            com = pickfirstcommand(ln);

            pid = fork();
            if (pid == 0) {
                int i = 0;
                char *tab[argseqLength(com->args) + 1];
                argseq *temp = com->args;
                do {
                    tab[i] = temp->arg;
                    i++;
                    temp = temp->next;
                } while (temp != com->args);
                tab[i] = NULL;

                execvp(com->args->arg, tab);

                switch (errno) {
                    case ENOENT:
                        writeErrorForProgram(com->args->arg, "no such file or directory\n");
                        break;
                    case EACCES:
                        writeErrorForProgram(com->args->arg, "permission denied\n");
                        break;
                    default:
                        writeErrorForProgram(com->args->arg, "exec error\n");
                        break;
                }

                return EXEC_FAILURE;
            } else {
                waitpid(pid, &wStatus, 0);
            }
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

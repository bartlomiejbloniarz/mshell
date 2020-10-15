#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <string.h>

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
	int readSize, wStatus;
	pid_t pid;

	char buf[MAX_LINE_LENGTH+1];

    write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));
	while ((readSize = read(STDIN, buf, MAX_LINE_LENGTH))){
	    buf[readSize] = 0;
		ln = parseline(buf);

		if (ln==NULL){
            write(STDERR, SYNTAX_ERROR_STR, sizeof(SYNTAX_ERROR_STR));
            write(STDERR, "\n", sizeof("\n"));
		}

        com = pickfirstcommand(ln);

		pid = fork();
		if (pid==0){
		    int i=0;
		    char* tab[argseqLength(com->args)+1];
		    argseq* temp = com->args;
            do{
                tab[i]=temp->arg;
                i++;
                temp = temp->next;
            }while(temp!=com->args);
            tab[i]=NULL;
            execvp(com->args->arg, tab);

            switch (errno) {
                case ENOENT: writeErrorForProgram(com->args->arg, "no such file or directory\n"); break;
                case EACCES: writeErrorForProgram(com->args->arg, "permission denied\n"); break;
                default: writeErrorForProgram(com->args->arg, "exec error\n"); break;
            }

            return EXEC_FAILURE;
		}
		else{
		    waitpid(pid, &wStatus, 0);
		}
        write(STDOUT, PROMPT_STR, sizeof(PROMPT_STR));
	}

	write(STDOUT, "\n", sizeof("\n"));

	/*ln = parseline("ls -las | grep k | wc ; echo abc > f1 ;  cat < f2 ; echo abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);

	ln = parseline("sleep 3 &");
	printparsedline(ln);
	printf("\n");
	
	ln = parseline("echo  & abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);*/

	return 0;
}

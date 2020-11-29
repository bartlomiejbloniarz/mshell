#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include "myutils.h"
#include "myconfig.h"
#include "builtins.h"

int echo(char*[]);
int myExit(char *[]);
int lcd(char* []);
int lkill(char* []);
int lls(char* []);

builtin_pair builtins_table[]={
	{"exit",	    &myExit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",	    &lls},
    {"cd",       &lcd},
	{NULL,NULL}
};


int echo( char * argv[])
{
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int myExit(char* argv[]){
    if (argv[1])
        return FAILURE;
    exit(EXIT_SUCCESS);
}

int lcd(char* argv[]){
    if (argv[1]) {
        if (argv[2])
            return FAILURE;
        return chdir(argv[1]);
    }
    return chdir(getenv("HOME"));
}

int lkill(char* argv[]){
    if (!argv[1])
        return FAILURE;
    if (argv[2]) {
        if (argv[3])
            return FAILURE;
        if (!isNumber(argv[2]) || !isNumber(argv[1]+1))
            return FAILURE;
        return kill(atoi(argv[2]), atoi(argv[1] + 1));
    }
    if (!isNumber(argv[1]))
        return FAILURE;
    return kill(atoi(argv[1]), SIGTERM);
}

int lls(char* argv[]){
    char temp[FILENAME_MAX];
    if (argv[1] == NULL)
        getcwd(temp, FILENAME_MAX);
    else
        strcpy(temp, argv[1]);
    DIR* dir = opendir(temp);
    if (dir == NULL)
        return FAILURE;
    errno = 0;
    struct dirent* dire;
    while((dire = readdir(dir))) {
        if (dire->d_name[0]=='.')
            continue;
        int size = strlen(dire->d_name)+2;
        char tab[size];
        strcpy(tab, dire->d_name);
        tab[size-2] = '\n'; tab[size-1] = 0;
        safeWrite(STDOUT, tab, size-1);
    }
    if (errno)
        return FAILURE;
    return closedir(dir);
}

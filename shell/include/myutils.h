#ifndef SHELL_MYUTILS_H
#define SHELL_MYUTILS_H

#include "builtins.h"
#include "siparse.h"

int argseqLength(argseq*);

void writeErrorForProgram(char*, char*);

void writeSyntaxError();

int findEndLine(const char*, int, int);

void mystrcpy(char*, const char*);

builtin_pair* findInBuiltins(char*);

void argsTab(command*, char**);

char isNumber(char*);

#endif //SHELL_MYUTILS_H

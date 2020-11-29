#ifndef SHELL_MYUTILS_H
#define SHELL_MYUTILS_H

#include "builtins.h"
#include "siparse.h"

int safeWrite(int fd, const void* buf, size_t size);

int argseqLength(argseq*);

int writeErrorForProgram(char*, char*);

int writeSyntaxError();

int findEndLine(const char*, int, int);

builtin_pair* findInBuiltins(char*);

void argsTab(command*, char**);

char isNumber(const char*);

void swap(int*, int*);

int commandseqLength(commandseq*);

int printErrors(char*);

void writeTermOrKill(pid_t pid, int stat);

void strShift(char* str, int start);

#endif //SHELL_MYUTILS_H

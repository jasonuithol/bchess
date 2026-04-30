#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stdio.h>

extern int logging;

void openLog(void);
void closeLog(void);
void logg(char* format, ...);
void print(char* format, ...);
void error(char* format, ...) __attribute__((noreturn));

#endif

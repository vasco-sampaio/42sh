#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>

int echo(char *args);

int builtin_exit(char *args);

int cd(char *args);

int export(char *args);

int dot(char *args);

#endif /* !BUILTIN_H */

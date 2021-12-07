#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

int isnumeric(char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        if (str[i] < '0' || str[i] > '9')
            return 0;
        i++;
    }
    return 1;
}

int builtin_exit(char *args)
{
    while (args[0] == ' ')
        args++;
    if (!isnumeric(args))
    {
        fprintf(stderr, "42sh: exit: Illegal number: %s\n", args);
        return 2;
    }
    if (strcmp(args, "") == 0)
        return 0;
    int code = strtol(args, NULL, 10);
    return code % 256;
}
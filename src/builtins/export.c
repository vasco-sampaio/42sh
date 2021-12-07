#include <ast/ast.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

static int specific_separator(char c)
{
    if (c == ' ' || c == ';')
        return 1;
    return 0;
}

static void put_var(char *name, char *value)
{
    char *var = build_var(name, value);
    var_assign_special(var);
    free(var);
    setenv(name, value, 1);
}

int export(char *args)
{
    size_t i = 0;
    while (args[i] != '\0')
    {
        while (isblank(args[i]))
            i++;
        if (args[i] == '\0')
            return 0;
        if (args[i] != '_' && !isalpha(args[i]))
        {
            fprintf(stderr, "42sh: Syntax error: '%c' unexpected\n", args[i]);
            return 2;
        }
        size_t count = 0;
        while (args[i + count] != '\0' && !specific_separator(args[i + count])
               && args[i + count] != '=')
            count++;
        char *name = strndup(args + i, count);
        char *value = strdup("");
        i += count;
        count = 0;
        if (args[i] == '=')
        {
            i++; // Skip '='
            while (args[i + count] != '\0'
                   && !specific_separator(args[i + count]))
                count++;
            free(value);
            value = strndup(args + i, count);
        }
        i += count;
        put_var(name, value);
        free(name);
        free(value);
    }
    return 0;
}

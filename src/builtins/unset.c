#include <ast/ast.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

// opt[0] -> -f opt[1] -> v
static size_t parse_options(char *args, int *opt, size_t len)
{
    size_t i = 0;
    int found = 0;
    for (; i < len; ++i)
    {
        if (args[i] == '-')
        {
            found = 1;
            char option[3] = {
                0,
            };
            size_t j = i + 1;
            for (; j < len && args[j] != ' '; ++j)
            {
                if (j - i > 2)
                    return i;
                option[j - i - 1] = args[j];
            }

            if (!strcmp(option, "f"))
                opt[0] = 1;
            else if (!strcmp(option, "v"))
                opt[1] = 1;
            else if (!strcmp(option, "fv") || !strcmp(option, "vf"))
            {
                opt[0] = 1;
                opt[1] = 1;
            }
            else
                return i;
            i = j;
        }
        else if (args[i] != ' ' || (i < len - 1 && args[i + 1] == ' '))
        {
            if (!found)
                return 0;
            return i;
        }
    }
    if (!found)
        return 0;
    return i;
}

int unset(char *args)
{
    size_t len = strlen(args);

    // options[0] => -f
    // options[1] => -v
    int *options = zalloc(sizeof(int) * 2);
    int remove_env = 0;
    size_t begin = parse_options(args, options, len);
    if (options[0] == 1)
        remove_env = 1;
    if (!options[0])
        options[1] = 1;

    struct vec *vector = vec_init();
    int code = 0;
    for (size_t i = begin; i <= len; ++i)
    {
        if (args[i] == '\0' || args[i] == ' ')
        {
            if (vector->data[0] != '_' && !isalpha(vector->data[0]))
            {
                fprintf(stderr, "42sh: Syntax error: '%c' unexpected\n",
                        args[i]);
                code = 2;
                break;
            }
            if (options[0])
            {
                remove_function(vec_cstring(vector));
            }
            if (options[1])
            {
                unset_var(vec_cstring(vector));
                if (remove_env)
                    unsetenv(vector->data);
            }
            vec_reset(vector);
        }
        else
            vec_push(vector, args[i]);
    }

    free(options);
    vec_destroy(vector);
    free(vector);
    return code;
}
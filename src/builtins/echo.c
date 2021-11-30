#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

static size_t parse_options(char *args, int *opt, size_t len)
{
    size_t i = 0;
    for (; i < len; ++i)
    {
        if (args[i] == '-')
        {
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

            if (!strcmp(option, "n"))
                opt[0] = 1;
            else if (!strcmp(option, "e"))
                opt[1] = 1;
            else if (!strcmp(option, "ne") || !strcmp(option, "en"))
            {
                opt[0] = 1;
                opt[1] = 1;
            }
            else
                return i;
            i = j;
        }
        else if (args[i] != ' ' || (i < len - 1 && args[i + 1] == ' '))
            return i;
    }
    return i;
}

int echo(char *args)
{
    size_t len = strlen(args);

    // options[0] => -n
    // options[1] => -e
    int *options = zalloc(sizeof(int) * 2);
    size_t begin = parse_options(args, options, len);

    struct vec *vector = vec_init();

    for (size_t i = begin; i < len; ++i)
    {
        if ((args[i] == '\\') && options[1] && i < len - 1)
        {
            if (args[i + 1] == '\\')
                vec_push(vector, '\\');
            else if (args[i + 1] == 'n')
                vec_push(vector, '\n');
            else if (args[i + 1] == 't')
                vec_push(vector, '\t');
            ++i;
        }

        else
            vec_push(vector, args[i]);
    }
    // print vec->value
    if (!options[0])
        printf("%s\n", vec_cstring(vector));
    else
        printf("%s", vec_cstring(vector));

    free(options);
    vec_destroy(vector);
    free(vector);
    return 0;
}

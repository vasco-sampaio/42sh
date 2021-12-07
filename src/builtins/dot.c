#include <ctype.h>
#include <errno.h>
#include <io/cstream.h>
#include <parser/parser.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

static int contain_slash(char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '/')
            return 1;
    }
    return 0;
}

static FILE *f_open(char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "42sh: %s : not found\n", path);
        return NULL;
    }
    return file;
}

static int eval_file(FILE *file)
{
    struct cstream *cs = cstream_file_create(file, /* fclose_on_free */ true);
    enum error err;
    struct vec *line = vec_init();
    struct vec *final = NULL;
    struct parser *parser = create_parser();

    while (true)
    {
        // Read the next character
        int c;
        if ((err = cstream_pop(cs, &c)))
            return err;

        // If the end of file was reached, stop right there
        if (c == EOF)
        {
            final = vec_concat(final, line);
            final->size--;
            vec_push(final, '\0');
            vec_reset(line);
            break;
        }

        // If a newline was met, print the line
        if (c == '\n')
        {
            final = vec_concat(final, line);
            final->size--;
            vec_push(final, '\n');
            vec_push(final, '\0');
            vec_reset(line);
            continue;
        }

        // Otherwise, add the character to the line
        vec_push(line, c);
    }
    free(cs);
    vec_destroy(line);
    free(line);

    parser->lexer = lexer_create(vec_cstring(final));
    enum parser_state state = parsing(parser);
    int return_code = 0;
    int eval = ast_eval(parser->ast, &return_code);
    parser_free(parser);

    if (state != PARSER_OK)
    {
        vec_destroy(final);
        free(final);
        return 2;
    }

    vec_destroy(final);
    free(final);
    return eval;
}

int dot(char *args)
{
    size_t i = 0;
    while (args[i] != '\0' && isblank(args[i]))
        i++;
    int slashed = contain_slash(args + i);
    int return_code = 0;
    FILE *file = NULL;
    if (!slashed)
    {
        char *base_path = strdup(getenv("PATH"));
        char *path =
            zalloc((strlen(base_path) + strlen(args + i) + 2) * sizeof(char));
        sprintf(path, "%s/%s", base_path, args + i);
        file = f_open(path);
        if (!file)
            return_code = 2;
        free(base_path);
        free(path);
    }
    else
    {
        file = f_open(args + i);
        if (!file)
            return_code = 127;
    }
    if (return_code == 0)
    {
        return eval_file(file);
    }
    return return_code;
}
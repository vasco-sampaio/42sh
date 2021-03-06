#include <builtins/builtin.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <evalexpr/eval_exp.h>
#include <parser/parser.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/utils.h>
#include <utils/vec.h>

#include "ast.h"

#define BUFFER_SIZE 1024
#define NONE 0
#define SIMPLE 1
#define DOUBLE 2

static int is_valid(char *str)
{
    size_t len = strlen(str);
    if (str[len - 1] == ')')
    {
        str[len - 1] = '\0'; // Remove closing parenthesis
        return 1;
    }
    return 0;
}

int subshell(char *args)
{
    if (!is_valid(args))
    {
        fprintf(stderr, "42sh: Syntax error: end of file unexpected\n");
        return 2;
    }
    struct parser *parser = create_parser();
    parser->lexer = lexer_create(args);
    enum parser_state state = parsing(parser);
    if (state != PARSER_OK)
    {
        free(args);
        return 2;
    }
    int pid = fork();
    if (pid == 0)
    {
        int return_value = 0;
        ast_eval(parser->ast, &return_value);
        parser_free(parser);
        exit(return_value);
    }
    parser_free(parser);
    int wstatus;
    int cpid = waitpid(pid, &wstatus, 0);
    if (cpid == -1)
        errx(1, "Failed waiting for child\n%s", strerror(errno));
    struct list *ret = zalloc(sizeof(struct list));
    ret->name = strdup("?");
    ret->value = my_itoa(WEXITSTATUS(wstatus));
    ret->next = NULL;
    add_var(ret);
    return WEXITSTATUS(wstatus);
}

char *cmd_sub(char *str, size_t quote_pos, size_t quote_end, int is_dollar)
{
    char *cmd = strndup(str + quote_pos + 1, quote_end - quote_pos - 1);

    int fds[2];

    if (pipe(fds) == -1)
        errx(1, "Failed to create pipe file descriptors.");

    struct parser *parser = create_parser();
    parser->lexer = lexer_create(cmd);
    enum parser_state state = parsing(parser);

    if (state != PARSER_OK)
    {
        free(cmd);
        return NULL;
    }
    int pid = fork();
    if (pid == 0)
    {
        if (dup2(fds[1], STDOUT_FILENO) == -1)
            errx(1, "dup2 failed");
        close(fds[0]);
        int return_value = 0;
        ast_eval(parser->ast, &return_value);
        parser_free(parser);
        exit(return_value);
    }
    parser_free(parser);

    char *res = zalloc(sizeof(char) * BUFFER_SIZE);
    int size = 0;
    int capacity = BUFFER_SIZE;
    close(fds[1]);
    char buf[BUFFER_SIZE];
    ssize_t r;
    while ((r = read(fds[0], &buf, BUFFER_SIZE)) != 0)
    {
        if (r == EOF)
            break;
        size += r;
        if (size >= capacity)
        {
            capacity *= 2;
            res = realloc(res, capacity);
        }
        strncat(res, buf, r);
    }
    close(fds[0]);

    int wstatus;
    int cpid = waitpid(pid, &wstatus, 0);
    if (cpid == -1)
        errx(1, "Failed waiting for child\n%s", strerror(errno));
    struct list *ret = zalloc(sizeof(struct list));
    ret->name = strdup("?");
    ret->value = my_itoa(WEXITSTATUS(wstatus));
    ret->next = NULL;
    add_var(ret);

    while (size > 0 && res[size - 1] == '\n')
        res[--size] = 0;

    int spaces = 0;
    int i = quote_pos - is_dollar - 1;
    while (i >= 0 && strlen(cmd) == 0 && str[i--] == ' ')
        spaces++;

    char *before = strndup(str, quote_pos - is_dollar - spaces);
    char *after = strdup(str + quote_end + 1);
    char *new =
        zalloc(sizeof(char) * (strlen(before) + strlen(after) + size + 1));
    sprintf(new, "%s%s%s", before, res, after);

    free(before);
    free(after);
    free(res);
    free(cmd);

    return new;
}

char *substitute_cmds(char *s)
{
    if (strlen(s) == 0)
        return s;
    char *str = strdup(s);
    free(s);
    size_t i = 0;
    int context = NONE;
    while (str[i] != 0)
    {
        if (str[i] == '\'')
        {
            if (context == NONE)
                context = SIMPLE;
            else if (context == SIMPLE)
                context = NONE;
        }
        if (str[i] == '\"')
        {
            if (context == NONE)
                context = DOUBLE;
            else if (context == DOUBLE)
                context = NONE;
        }

        if (str[i] == '`' && context != SIMPLE && not_as_escape(str, i - 1))
        {
            char *next = strchr(str + i + 1, '`');
            if (next == NULL)
            {
                fprintf(stderr, "42sh: Syntax error: ` unmatched");
                free(str);
                return NULL;
            }

            char *tmp = cmd_sub(str, i, next - str, 0);
            if (tmp == NULL)
                return NULL;
            free(str);
            str = tmp;
            i = 0;
        }
        i++;
    }
    i = strlen(str) - 1;
    context = NONE;
    int closing = 0;
    int opening = 0;
    while (i > 0)
    {
        if (str[i] == '\'')
        {
            if (context == NONE)
                context = SIMPLE;
            else if (context == SIMPLE)
                context = NONE;
        }
        if (str[i] == '\"')
        {
            if (context == NONE)
                context = DOUBLE;
            else if (context == DOUBLE)
                context = NONE;
        }
        if (str[i] == ')')
            closing++;
        if (str[i] == '(')
            opening++;

        if (str[i] == '(' && str[i - 1] == '$' && not_as_escape(str, i - 2)
            && str[i + 1] != '(')
        {
            if (closing == 0)
            {
                fprintf(stderr, "42sh: Syntax error: ( unmatched");
                free(str);
                return NULL;
            }
            char *next = str + i;
            for (int j = 0; j < closing - (closing - opening); j++)
            {
                next = strchr(next + 1, ')');
                if (next == NULL)
                {
                    fprintf(stderr, "42sh: Syntax error: ( unmatched");
                    free(str);
                    return NULL;
                }
            }
            char *tmp = cmd_sub(str, i, next - str, 1);
            if (tmp == NULL)
                return NULL;
            free(str);
            str = tmp;
            closing = 0;
            opening = 0;
            i = strlen(str);
        }
        i--;
    }
    return str;
}

char *arithmetic_exp(char *cmd)
{
    size_t equ_index = 0;
    while (cmd[equ_index] != '\0' && (cmd[equ_index] != '='))
        ++equ_index;
    int i = equ_index + 1;

    if (equ_index == strlen(cmd))
    {
        int doll_idx = 0;
        while (cmd[doll_idx] != '\0' && cmd[doll_idx] != '$')
            doll_idx++;
        i = doll_idx;
        equ_index = doll_idx;
    }

    if (cmd[equ_index] == '\0' || cmd[i] == '\0' || cmd[i++] != '$')
        return cmd;
    if (cmd[i] == '\0' || cmd[i] != '(')
        return cmd;
    if (cmd[i + 1] == '\0' || cmd[i + 1] != '(')
        return cmd;
    int res = eval_exp(cmd + i);
    if (res == INT_MIN)
    {
        fprintf(stderr, "42sh: Invalid arithmetic expression\n");
        free(cmd);
        return NULL;
    }
    char *to_sprintf = zalloc(sizeof(char) * (equ_index + 21));
    char *before_eq = NULL;
    if (cmd[equ_index] == '=')
        before_eq = strndup(cmd, equ_index + 1);
    else
        before_eq = strndup(cmd, equ_index);
    sprintf(to_sprintf, "%s%d", before_eq, res);
    free(cmd);
    free(before_eq);
    return to_sprintf;
}

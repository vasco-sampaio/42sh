#include <builtins/builtin.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <parser/parser.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/utils.h>
#include <utils/vec.h>

#include "ast.h"

static char *is_valid(char *str)
{
    char *duped = strdup(str);
    size_t len = strlen(duped);
    if (duped[len - 1] == '}')
    {
        duped[len - 1] = '\0'; // Remove closing bracket
        return duped;
    }
    free(duped);
    return NULL;
}

int cmdblock(char *args)
{
    char *blk = is_valid(args);
    if (!blk)
    {
        fprintf(stderr, "42sh: Syntax error: end of file unexecpected\n");
        return 2;
    }

    struct parser *parser = create_parser();
    parser->lexer = lexer_create(blk);
    enum parser_state state = parsing(parser);
    int return_code = 0;
    if (state != PARSER_OK)
        return_code = 2;
    return_code = ast_eval(parser->ast, &return_code);
    global->parsers_to_free[global->nb_parsers++] = parser;
    free(blk);
    return return_code;
}

int add_function(struct ast *ast)
{
    struct function *new = zalloc(sizeof(struct function));
    new->name = strdup(vec_cstring(ast->val));
    new->body = ast->left;
    new->next = global->functions;
    global->functions = new;
    return 0;
}

void remove_function(char *name)
{
    struct function *fcs = global->functions;
    struct function *prev = NULL;
    while (fcs != NULL)
    {
        if (strcmp(fcs->name, name) == 0)
        {
            if (prev == NULL)
            {
                struct function *save = global->functions;
                global->functions = global->functions->next;
                free(save->name);
                free(save);
                return;
            }
            prev->next = fcs->next;
            free(fcs->name);
            free(fcs);
            return;
        }
        prev = fcs;
        fcs = fcs->next;
    }
}

int eval_func(char *cmd)
{
    struct function *fs = global->functions;
    char *params = strchr(cmd, ' ');
    char *name = strndup(cmd, params - cmd);

    while (fs)
    {
        if (!strcmp(name, fs->name))
            break;
        fs = fs->next;
    }
    free(name);
    if (!fs)
    {
        return -1;
    }

    int nb_params = 1;
    if (params)
        params++;
    char *save_params = strdup("");
    if (!params)
        nb_params = 0;
    while (nb_params <= 9 && nb_params != 0)
    {
        char *current_param = strchr(params, ' ');
        if (!current_param)
            current_param = params + strlen(params);
        if (current_param == params)
        {
            nb_params = 0;
            break;
        }
        char *nb_params_alloc = my_itoa(nb_params);
        char *tmp = strndup(params, current_param - params);

        while (*tmp && isblank(*tmp))
            ++tmp;
        if (strlen(tmp))
        {
            char *save_param_tmp =
                calloc(strlen(save_params) + strlen(tmp) + 2, sizeof(char));
            if (!strcmp("", save_params))
                sprintf(save_param_tmp, "%s", tmp);
            else
                sprintf(save_param_tmp, "%s %s", save_params, tmp);
            free(save_params);
            save_params = save_param_tmp;
        }
        push_front(nb_params_alloc, tmp);
        free(nb_params_alloc);
        params = current_param + 1;
        ++nb_params;
        if (!(*current_param))
            break;
    }

    if (save_params == NULL)
        save_params = "";
    if (nb_params > 0)
        nb_params--;
    push_front("*", strdup(save_params));
    push_front("@", strdup(save_params));
    push_front("#", my_itoa(nb_params));
    free(save_params);

    int ret = 0;
    int return_val = ast_eval(fs->body, &ret);

    unset_var("*");
    unset_var("@");
    unset_var("#");
    for (int i = 1; i <= nb_params; ++i)
    {
        char *itoaed = my_itoa(i);
        unset_var(itoaed);
        free(itoaed);
    }

    return return_val;
}
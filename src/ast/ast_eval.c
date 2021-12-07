#include <builtins/builtin.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/utils.h>
#include <utils/vec.h>

#include "ast.h"
#include "redirection.h"

/**
 * \brief The number of builtins commands
 */
#define BLT_NB 5

/**
 * \brief The number of redirection operators
 */
#define REDIR_NB 7

enum cmd_mode current_mode = 0;
struct list *vars = NULL;

/**
 * \brief Split a string into an array in which the
 * first element is the command.
 * @param cmd: the string to spilit
 * @param size: the size of the returned array
 * @return: the splited array
 */

static char **split_in_array(char *cmd, int *size)
{
    char **args = xmalloc(sizeof(char *) * strlen(cmd));
    int index = 0;
    int i = 0;
    while (cmd[i] != 0)
    {
        while (cmd[i] != 0 && cmd[i] == ' ')
            i++;
        index = i;
        while (cmd[i] != 0 && cmd[i] != ' ')
            i++;
        if (i != index)
            args[(*size)++] = strndup(cmd + index, i - index);
    }
    args[*size] = NULL;
    return args;
}

/**
 * \brief Execute a command in a sub-process
 * @param cmd: The command to execute
 * @return: return if the command fail or succeed
 */
static int fork_exec(char *cmd)
{
    int size = 0;
    char **args = split_in_array(cmd, &size);
    int pid = fork();
    if (pid == -1)
        errx(1, "Failed to fork\n");

    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            fprintf(stderr, "Command not found: '%s'\n", args[0]);
            for (int i = 0; i < size; i++)
                free(args[i]);
            free(args);
            exit(127);
        }
    }

    for (int i = 0; i < size; i++)
        free(args[i]);
    free(args);
    int wstatus;
    int cpid = waitpid(pid, &wstatus, 0);

    if (cpid == -1)
        errx(1, "Failed waiting for child\n%s", strerror(errno));
    return WEXITSTATUS(wstatus);
}

int cmd_exec(char *cmd)
{
    char *builtins[] = { "echo", "exit", "cd", "export", "." };
    commands cmds[BLT_NB] = { &echo, &builtin_exit, &cd, &export, &dot };

    int arg_index = 0;
    char *cmd_name = getcmdname(cmd, &arg_index);
    if (cmd_name[arg_index] == 0)
        arg_index--; // handle \0 for empty args
    int i = 0;
    while (i < BLT_NB)
    {
        if (strcmp(cmd_name, builtins[i]) == 0)
        {
            free(cmd_name);
            int return_code = cmds[i](cmd + arg_index + 1);
            if (i == 1)
                current_mode = EXIT;
            return return_code;
        }
        i++;
    }
    free(cmd_name);
    return fork_exec(cmd);
}

static int eval_pipe(struct ast *ast)
{
    int fds[2];

    if (pipe(fds) == -1)
        errx(1, "Failed to create pipe file descriptors.");

    int out = dup(STDOUT_FILENO);

    if (dup2(fds[1], STDOUT_FILENO) == -1)
        errx(1, "dup2 failed");
    int return_code = 0;
    ast_eval(ast->left, &return_code);

    dup2(out, STDOUT_FILENO);
    close(out);

    int in = dup(STDIN_FILENO);

    if (dup2(fds[0], STDIN_FILENO) == -1)
        errx(1, "dup2 failed");
    close(fds[1]);
    int res = ast_eval(ast->right, &return_code);

    dup2(in, STDIN_FILENO);
    close(in);
    close(fds[0]);
    close(fds[1]);

    return res;
}

int exec_redir(struct ast *ast)
{
    char *redirs_name[] = { ">", "<", ">>", ">&", ">|", "<&", "<>" };
    redirs_funcs redirs[REDIR_NB] = {
        &redir_simple_left,    &redir_simple_right, &redir_double_left,
        &redir_ampersand_left, &redir_simple_left,  &redir_ampersand_right,
        &redir_left_right
    };

    size_t i = 0;
    int fd = -1;
    if (!is_redirchar(ast->val->data[i])) // TODO: handle not digit case
        fd = ast->val->data[i++] - '0';
    while (is_redirchar(ast->val->data[i]))
        i++;
    if (fd == -1 && i > 2)
        return 2; // Not valid
    char *redir_mode = NULL;
    if (fd != -1)
        redir_mode = strndup(ast->val->data + 1, i - 1);
    else
        redir_mode = strndup(ast->val->data, i);
    while (isspace(ast->val->data[i])) // Skip spaces before WORD
        i++;
    char *right = strdup(ast->val->data + i); // Skip redir operator
    int return_code = -1;
    for (size_t index = 0; index < REDIR_NB; index++)
    {
        if (strcmp(redirs_name[index], redir_mode) == 0)
        {
            return_code = redirs[index](ast->left, fd, right);
            break;
        }
    }
    free(redir_mode);
    free(right);
    if (ast->right)
        return exec_redir(ast->right);
    return return_code;
}

static void set_var(char *var, struct ast *ast)
{
    if (ast == NULL)
        return;
    ast->var = strdup(var);
    set_var(var, ast->left);
    set_var(var, ast->right);
}

static void set_replace(char *value, struct ast *ast)
{
    if (ast == NULL)
        return;
    if (ast->replace)
        free(ast->replace);
    ast->replace = strdup(value);
    set_replace(value, ast->left);
    set_replace(value, ast->right);
}

void free_var(struct list *var)
{
    free(var->name);
    free(var->value);
    free(var);
}

int ast_eval(struct ast *ast, int *return_code)
{
    if (!ast)
        return 0;
    if (current_mode == EXIT)
        return *return_code;
    if (ast->type == AST_OR)
    {
        int left = ast_eval(ast->left, return_code);
        if (left == 0 || !ast->right)
            return left;
        return ast_eval(ast->right, return_code);
    }
    if (ast->type == AST_ROOT)
    {
        int left = ast_eval(ast->left, return_code);
        if (!ast->right)
            return left;
        return ast_eval(ast->right, return_code);
    }
    else if (ast->type == AST_IF || ast->type == AST_ELIF)
    {
        int test_cond = ast_eval(ast->cond, return_code);
        if (current_mode == EXIT)
        {
            *return_code = test_cond;
            return test_cond;
        }
        if (!test_cond) // true
            return ast_eval(ast->left, return_code);
        else
            return ast_eval(ast->right, return_code);
    }
    else if (ast->type == AST_THEN || ast->type == AST_ELSE)
        return ast_eval(ast->left, return_code);
    else if (ast->type == AST_CMD)
    {
        int res = 0;
        char *cmd2 = strdup(ast->val->data);

        cmd2 = expand_vars(cmd2, ast->var, ast->replace);
        cmd2 = remove_quotes(cmd2);
        if (!is_var_assign(cmd2))
            res = cmd_exec(cmd2);
        free(cmd2);
        *return_code = res;
        int i = 0;
        char *command_name = getcmdname(ast->val->data, &i);
        if (strcmp(command_name, ".") == 0 && res != 0)
            current_mode = EXIT;
        free(command_name);
        if (!ast->left)
        {
            char *value = my_itoa(res);
            char *var = build_var("?", value);
            var_assign_special(var);
            free(var);
            free(value);
            return res;
        }
        res = ast_eval(ast->left, return_code);
        char *value = my_itoa(res);
        char *var = build_var("?", value);
        var_assign_special(var);
        free(var);
        free(value);
        return res;
    }
    else if (ast->type == AST_REDIR)
    {
        char *tmp = expand_vars(ast->val->data, NULL, NULL);
        tmp = remove_quotes(tmp);
        ast->val->data = tmp;
        ast->val->size = strlen(tmp);
        ast->val->capacity = strlen(tmp) + 1;
        return exec_redir(ast);
    }
    else if (ast->type == AST_PIPE)
        return eval_pipe(ast);
    else if (ast->type == AST_AND)
    {
        int left = ast_eval(ast->left, return_code);
        if (left != 0)
            return left;
        return ast_eval(ast->right, return_code);
    }
    else if (ast->type == AST_NEG)
        return !ast_eval(ast->left, return_code);
    else if (ast->type == AST_WHILE)
    {
        int a = 0;
        while (ast_eval(ast->cond, return_code) == 0)
        {
            a = ast_eval(ast->left, return_code);
        }
        return a;
    }
    else if (ast->type == AST_UNTIL)
    {
        int a = 0;
        while (ast_eval(ast->cond, return_code) != 0)
        {
            a = ast_eval(ast->left, return_code);
        }
        return a;
    }
    else if (ast->type == AST_FOR)
    {
        set_var(ast->val->data, ast->left);
        int ret_code = 0;
        char **total = zalloc(100000);
        size_t size = 0;
        for (size_t i = 0; i < ast->size; i++)
        {
            char *s = strdup(ast->list[i]);
            s = expand_vars(s, NULL, NULL);
            if (s[0] != '\"')
            {
                char **args = zalloc(sizeof(char *) * strlen(s));
                char *save = NULL;
                char *val = strtok_r(s, " \n", &save);
                int index = 0;
                args[index++] = val;
                while ((val = strtok_r(NULL, " \n", &save)) != NULL)
                    args[index++] = val;
                for (int i = 0; i < index; i++)
                    total[size++] = strdup(args[i]);
                free(args);
            }
            else
                total[size++] = strdup(s);
            free(s);
        }
        for (size_t i = 0; i < size; ++i)
        {
            set_replace(total[i], ast->left);
            ret_code = ast_eval(ast->left, return_code);
            free(total[i]);
        }
        free(total);
        return ret_code;
    }
    else
    {
        printf("ast->type = %d\n", ast->type);
        fprintf(stderr, "ast_eval: node type not known\n");
        return 2;
    }
}

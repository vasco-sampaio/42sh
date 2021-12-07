#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>

struct list
{
    char *name;
    char *value;
    struct list *next;
};

extern struct list *vars;

/**
 * \brief Possible nodes types for ast structure.
 */
enum ast_type
{
    AST_ROOT,
    AST_IF,
    AST_THEN,
    AST_ELIF,
    AST_ELSE,
    AST_CMD,
    AST_REDIR,
    AST_PIPE,
    AST_AND,
    AST_OR,
    AST_NEG,
    AST_WHILE,
    AST_UNTIL,
    AST_FOR
};

/**
 * \brief Possible modes for ast evaluation
 */
enum cmd_mode
{
    EXIT = 1,
    BREAK = 2,
    CONTINUE = 3
};

/**
 * \brief Structure for ast.
 * @details: cond use for ifs conditions
 */
struct ast
{
    enum ast_type type;
    char **list;
    size_t size;
    size_t capacity;
    char *var;
    char *replace;
    struct vec *val;
    struct ast *cond;
    struct ast *left;
    struct ast *right;
};

/**
 * \brief Array of pointers to builtins commands.
 */
typedef int (*commands)(char *args);

/**
 * \brief  Functions pointers arrays to redirection functions
 * @param left: the left part of the redirection
 * @param fd: the file descriptor (STDOUT by default)
 * @param right: the right part of the redirection
 * @return value: return code of exec, -1 on failure
 */
typedef int (*redirs_funcs)(struct ast *left, int fd, char *right);

/**
 * \brief Evaluate the ast and execute commands.
 */
int ast_eval(struct ast *ast, int *return_code);

void free_var(struct list *var);

struct ast *create_ast(enum ast_type type);

void ast_free(struct ast *ast);

void add_to_list(struct ast *ast, char *str);

void pretty_print(struct ast *ast);

/**
 * \brief Choose to execute builtin commands or
 * not builtins.
 * @param cmd: the command to execute
 * @return: return if the command fail or succeed
 */
int cmd_exec(char *cmd);

char *my_strstr(char *str, char *var);

char *replace_vars(char *str, char *var, char *replace);

char *expand_vars(char *str, char *var, char *var_rep);

char *remove_quotes(char *str);

void add_var(struct list *new);

int is_var_assign(char *str);

void set_special_vars(void);

char *build_var(char *name, char *value);

void var_assign_special(char *str);

char *my_itoa(int n);

// char *remove_vars(char *str, char *exclude);

#endif /* ! AST_H */

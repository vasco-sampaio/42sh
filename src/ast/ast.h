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

struct function
{
    char *name;
    struct ast *body;
    struct function *next;
};

struct global
{
    struct mode *current_mode;
    struct list *vars;
    struct function *functions;
    struct list *save_vars;
    struct parser *parsers_to_free[100];
    int nb_parsers;
};

extern struct global *global;

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
    AST_FOR,
    AST_SUBSHELL,
    AST_CMDBLOCK,
    AST_FUNCTION,
    AST_BREAK,
    AST_CONTINUE,
    AST_CASE
};

/**
 * \brief Possible modes for ast evaluation
 */
enum cmd_mode
{
    NORMAL = 0,
    EXIT = 1,
    BREAK = 2,
    CONTINUE = 3
};

struct mode
{
    enum cmd_mode mode;
    size_t nb;
    int depth;
};

struct cas
{
    char *pattern;
    struct ast *ast;
    struct cas *next;
};

/**
 * \brief Structure for ast.
 * @details: cond use for ifs conditions
 */
struct ast
{
    int is_loop;

    enum ast_type type;

    char **list;
    size_t size;
    size_t capacity;

    char *var;
    char *replace;

    char *word;
    struct cas *cas;

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

void unset_var(char *name);

/**
 * \brief Exectue args in a new process
 */
int subshell(char *args);

/**
 * \brief Execute cmd substitution
 */
char *cmd_sub(char *str, size_t quote_pos, size_t quote_end, int is_dollar);

/**
 * \brief Execute arithmetic expansion
 * if suceed replace the expression by its value
 * on failure return NULL
 */
char *arithmetic_exp(char *cmd);

char *substitute_cmds(char *s);

/**
 * \brief Exectue args in a whole block
 */
int cmdblock(char *args);

/**
 * \brief Add a function in the global list
 */
int add_function(struct ast *ast);

/**
 * \brief Execute a local function from global function list
 * Returns -1 if function not found
 */
int eval_func(char *cmd);

/**
 * \brief Push a variable at the beginning of the var list
 */
void push_front(char *name, char *value);

/**
 * \brief Remove a variable from the global list
 */
void remove_function(char *name);

// char *remove_vars(char *str, char *exclude);
int handle_case(struct ast *ast);

#endif /* ! AST_H */

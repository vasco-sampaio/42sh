#ifndef AST_H
#define AST_H

#include <stdbool.h>

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
    AST_OR
};

/**
 * \brief Structure for ast.
 * @details: cond use for ifs conditions
 */
struct ast
{
    enum ast_type type;
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
 * \brief Evaluate the ast and execute commands.
 */
int ast_eval(struct ast *ast);

struct ast *create_ast(enum ast_type type);

void ast_free(struct ast *ast);

void pretty_print(struct ast *ast);

#endif /* ! AST_H */

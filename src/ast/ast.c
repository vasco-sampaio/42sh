#include "ast.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/alloc.h>
#include <utils/vec.h>

struct ast *create_ast(enum ast_type type)
{
    struct ast *new = zalloc(sizeof(struct ast));
    new->type = type;
    new->left = NULL;
    new->right = NULL;
    new->val = NULL;
    new->cond = NULL;
    return new;
}

void ast_free(struct ast *ast)
{
    if (!ast)
        return;

    if (ast->val)
    {
        free(ast->val->data);
        free(ast->val);
    }
    ast_free(ast->cond);
    ast_free(ast->left);
    ast_free(ast->right);
    free(ast);
}

static void pretty_rec(struct ast *ast)
{
    if (!ast)
        return;
    if (ast->type == AST_ROOT)
    {
        pretty_rec(ast->left);
        pretty_rec(ast->right);
    }
    else if (ast->type == AST_CMD)
    {
        printf("command \"");
        vec_print(ast->val);
        printf("\" ");
        pretty_rec(ast->left);
    }
    else if (ast->type == AST_IF || ast->type == AST_ELIF)
    {
        if (ast->type == AST_IF)
            printf("if { ");
        else
            printf("elif { ");
        pretty_rec(ast->cond);
        printf("}; ");
        pretty_rec(ast->left);
        pretty_rec(ast->right);
    }
    else if (ast->type == AST_THEN)
    {
        printf("then { ");
        pretty_rec(ast->left);
        printf("} ");
    }
    else if (ast->type == AST_ELSE)
    {
        printf("else { ");
        pretty_rec(ast->left);
        printf("} ");
    }
    else if (ast->type == AST_REDIR)
    {
        pretty_rec(ast->left);
        printf("redir %s ", ast->val->data);
    }
    else if (ast->type == AST_PIPE)
    {
        pretty_rec(ast->left);
        printf("| ");
        pretty_rec(ast->right);
    }
    else if (ast->type == AST_OR || ast->type == AST_AND)
    {
        pretty_rec(ast->left);
        if (ast->type == AST_OR)
            printf("|| ");
        else
            printf("&& ");
        pretty_rec(ast->right);
    }
    else
        printf("pretty-print : Unknown node type\n");
}

void pretty_print(struct ast *ast)
{
    pretty_rec(ast);
    printf("\n");
}

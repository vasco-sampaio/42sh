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
    new->size = 0;
    new->capacity = 5;
    new->list = zalloc(sizeof(char *) * new->capacity);
    new->var = NULL;
    new->replace = NULL;
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
    for (size_t i = 0; i < ast->size; ++i)
        free(ast->list[i]);
    free(ast->list);
    if (ast->var)
        free(ast->var);
    if (ast->replace)
        free(ast->replace);
    ast_free(ast->cond);
    ast_free(ast->left);
    ast_free(ast->right);
    free(ast);
}

void add_to_list(struct ast *ast, char *str)
{
    if (ast->size == ast->capacity)
    {
        ast->capacity *= 2;
        ast->list = xrealloc(ast->list, ast->capacity);
    }
    ast->list[ast->size++] = strdup(str);
}

static void pretty_rec(struct ast *ast)
{
    if (!ast)
        return;
    if (ast->type == AST_ROOT)
    {
        printf("root ");
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
        pretty_rec(ast->right);
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
    else if (ast->type == AST_NEG)
    {
        printf("! ");
        pretty_rec(ast->left);
    }
    else if (ast->type == AST_WHILE || ast->type == AST_UNTIL)
    {
        if (ast->type == AST_WHILE)
            printf("while { ");
        else
            printf("until { ");
        pretty_rec(ast->cond);
        printf("}; do {");
        pretty_rec(ast->left);
        pretty_rec(ast->right);
        printf("}; done ");
    }
    else if (ast->type == AST_FOR)
    {
        printf("for { %s ", ast->val->data);
        if (ast->size != 0)
        {
            printf("in ");
            for (size_t i = 0; i < ast->size; ++i)
                printf("%s ", ast->list[i]);

            printf("} ");
        }
        printf("do ");
        pretty_rec(ast->left);
        printf("done ");
    }
    else
        printf("pretty-print : Unknown node type\n");
}

void pretty_print(struct ast *ast)
{
    pretty_rec(ast);
    printf("\n");
}

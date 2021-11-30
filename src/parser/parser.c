#include "parser.h"

#include <ast/ast.h>
#include <err.h>
#include <lexer/lexer.h>
#include <lexer/token.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <utils/vec.h>

enum parser_state parse_else_clause(struct parser *parser, struct ast **ast);
enum parser_state parse_elif(struct parser *parser, struct ast **ast);
enum parser_state parse_pipe(struct parser *parser, struct ast **ast);

static enum parser_state handle_parse_error(enum parser_state state,
                                            struct parser *parser)
{
    warnx("Parser error");
    ast_free(parser->ast);
    parser->ast = NULL;
    return state;
}

void parser_free(struct parser *parser)
{
    lexer_free(parser->lexer);
    ast_free(parser->ast);
    free(parser);
}

struct parser *create_parser()
{
    struct parser *parser = zalloc(sizeof(struct parser));
    parser->lexer = NULL;
    parser->ast = NULL;
    return parser;
}

static enum parser_state parse_redir(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_REDIR)
        return PARSER_ABSENT;
    struct ast *placeholder = create_ast(AST_REDIR);
    placeholder->left = *ast;
    *ast = placeholder;
    (*ast)->val = vec_init();
    (*ast)->val->data = strdup(tok->value);
    (*ast)->val->size = strlen(tok->value);
    (*ast)->val->capacity = strlen(tok->value) + 1;
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    return PARSER_OK;
}

static enum parser_state parse_prefix(struct parser *parser, struct ast **ast)
{
    return parse_redir(parser, ast);
}

static enum parser_state parse_element(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type == TOKEN_ERROR)
    {
        free(*ast);
        return PARSER_PANIC;
    }
    if (tok->type == TOKEN_WORD || tok->type == TOKEN_ECHO)
    {
        struct vec *tmp = vec_init();
        tmp->data = strdup(tok->value);
        tmp->size = strlen(tok->value);
        tmp->capacity = tmp->size + 1;
        (*ast)->val = vec_concat((*ast)->val, tmp);
        vec_destroy(tmp);
        free(tmp);
        tok = lexer_pop(parser->lexer);
        token_free(tok);
        tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;
        if (tok->type == TOKEN_WORD || tok->type == TOKEN_ECHO)
        {
            (*ast)->val->size--;
            vec_push((*ast)->val, ' ');
            vec_push((*ast)->val, '\0');
        }
        return PARSER_OK;
    }
    return parse_redir(parser, ast);
}

static enum parser_state parse_simple_command(struct parser *parser,
                                              struct ast **ast)
{
    struct ast *new = create_ast(AST_CMD);
    enum token_type tok_type = lexer_peek(parser->lexer)->type;
    if (tok_type == TOKEN_ERROR)
    {
        free(new);
        return PARSER_PANIC;
    }
    int prefix_count = 0;
    while (1) // (prefix)*
    {
        enum parser_state state = parse_prefix(parser, &new);
        if (state == PARSER_PANIC)
        {
            ast_free(new);
            return PARSER_PANIC;
        }
        if (state == PARSER_ABSENT)
            break;
        prefix_count++;
    }
    int element_count = 0;
    while (1)
    {
        enum parser_state state = parse_element(parser, &new);
        if (state == PARSER_PANIC)
        {
            ast_free(new);
            return PARSER_PANIC;
        }
        if (state == PARSER_ABSENT && element_count == 0)
        {
            ast_free(new);
            return PARSER_ABSENT;
        }
        if (state == PARSER_ABSENT)
            break;
        element_count++;
    }
    if (element_count == 0 && prefix_count == 0)
    {
        ast_free(new);
        return PARSER_ABSENT;
    }
    *ast = new;
    return PARSER_OK;
}

static enum parser_state parse_and_or(struct parser *parser, struct ast **ast)
{
    enum parser_state state = parse_pipe(parser, ast);
    if (state != PARSER_OK)
        return state;

    while (1)
    {
        struct token *tok = lexer_peek(parser->lexer);
        if (tok->type != TOKEN_AND && tok->type != TOKEN_OR)
            break;
        enum token_type tok_type = tok->type;
        tok = lexer_pop(parser->lexer);
        token_free(tok);

        // parsing (/n)*
        while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
        {
            lexer_pop(parser->lexer);
            token_free(tok);
        }
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;

        struct ast *and_or_node =
            create_ast(tok_type == TOKEN_AND ? AST_AND : AST_OR);

        and_or_node->left = *ast;
        *ast = and_or_node;

        state = parse_pipe(parser, &((*ast)->right));
        if (state != PARSER_OK)
            return state;
    }
    return PARSER_OK;
}

static enum parser_state parse_compound_list(struct parser *parser,
                                             struct ast **ast)
{
    struct token *tok;
    while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
    {
        lexer_pop(parser->lexer);
        token_free(tok);
    }
    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;

    enum parser_state state = parse_and_or(parser, ast);
    if (state != PARSER_OK)
        return state;

    while (42)
    {
        struct ast *root = create_ast(AST_ROOT);
        root->left = *ast;
        tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
        {
            ast_free(root);
            return PARSER_PANIC;
        }
        if (tok->type != TOKEN_NEWL && tok->type != TOKEN_SEMIC)
            break;
        lexer_pop(parser->lexer);
        token_free(tok);
        while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
        {
            lexer_pop(parser->lexer);
            token_free(tok);
        }
        if (tok->type == TOKEN_ERROR)
        {
            ast_free(root);
            return PARSER_PANIC;
        }

        state = parse_and_or(parser, &(root->right));
        if (state == PARSER_ABSENT)
        {
            *ast = root;
            break;
        }
        else if (state == PARSER_PANIC)
        {
            ast_free(root);
            return state;
        }
        *ast = root;
    }
    return PARSER_OK;
}

enum parser_state parse_else_clause(struct parser *parser, struct ast **ast)
{
    // checking for else token
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;
    if (tok->type != TOKEN_ELSE)
    {
        if (tok->type != TOKEN_ELIF)
            return PARSER_ABSENT;
        return parse_elif(parser, ast);
    }
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    struct ast *new = create_ast(AST_ELSE);
    *ast = new;

    // getting commands for else
    return parse_compound_list(parser, &((*ast)->left));
}

enum parser_state parse_elif(struct parser *parser, struct ast **ast)
{
    // skip elif (we know it is here)
    struct token *tok = lexer_pop(parser->lexer);
    token_free(tok);
    struct ast *new = create_ast(AST_ELIF);
    *ast = new;

    // getting condition for elif
    enum parser_state state = parse_compound_list(parser, &((*ast)->cond));
    if (state != PARSER_OK)
        return state;

    // checking for then token
    if (lexer_peek(parser->lexer)->type != TOKEN_THEN)
        return PARSER_PANIC;
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    new = create_ast(AST_THEN);
    (*ast)->left = new;

    // getting commands for then
    state = parse_compound_list(parser, &((*ast)->left->left));
    if (state != PARSER_OK)
        return state;

    // launch else_clause function
    state = parse_else_clause(parser, &((*ast)->right));
    if (state == PARSER_PANIC)
        return state;

    return PARSER_OK;
}

static enum parser_state parse_rule_if(struct parser *parser, struct ast **ast)
{
    // checking for if token
    if (lexer_peek(parser->lexer)->type != TOKEN_IF)
        return PARSER_PANIC;
    struct token *tok = lexer_pop(parser->lexer);
    token_free(tok);

    struct ast *new = create_ast(AST_IF);
    *ast = new;

    // getting condition for if
    enum parser_state state = parse_compound_list(parser, &((*ast)->cond));
    if (state != PARSER_OK)
        return state;

    // checking for then token
    if (lexer_peek(parser->lexer)->type != TOKEN_THEN)
        return PARSER_PANIC;

    tok = lexer_pop(parser->lexer);
    token_free(tok);

    new = create_ast(AST_THEN);
    (*ast)->left = new;

    // getting commands for then
    state = parse_compound_list(parser, &((*ast)->left->left));
    if (state != PARSER_OK)
        return state;

    // launch else_clause function
    state = parse_else_clause(parser, &((*ast)->right));
    if (state == PARSER_PANIC)
        return state;

    // checking for fi token
    if (lexer_peek(parser->lexer)->type != TOKEN_FI)
        return PARSER_PANIC;
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    return PARSER_OK;
}

static enum parser_state parse_shell_command(struct parser *parser,
                                             struct ast **ast)
{
    return parse_rule_if(parser, ast);
}

static enum parser_state parse_command(struct parser *parser, struct ast **ast)
{
    enum parser_state state = parse_shell_command(parser, ast);
    if (state != PARSER_OK)
    {
        enum parser_state state2 = parse_simple_command(parser, ast);
        return state2;
    }

    struct token *tok = lexer_peek(parser->lexer);
    while (tok->type == TOKEN_REDIR)
    {
        parse_redir(parser, ast);
        tok = lexer_pop(parser->lexer);
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;
        token_free(tok);
        tok = lexer_peek(parser->lexer);
    }
    return state;
}

enum parser_state parse_pipe(struct parser *parser, struct ast **ast)
{
    // parsing command
    enum parser_state state = parse_command(parser, ast);
    if (state != PARSER_OK)
        return state;
    // parsing ( '|' ( /n )* command )*
    while (1)
    {
        // parsing '|'
        struct token *tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;
        if (tok->type != TOKEN_PIPE)
            break;
        struct ast *pipe_node = create_ast(AST_PIPE);

        pipe_node->left = *ast;
        *ast = pipe_node;
        lexer_pop(parser->lexer);
        token_free(tok);

        // parsing (/n)*
        while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
        {
            lexer_pop(parser->lexer);
            token_free(tok);
        }

        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;

        // parsing command
        state = parse_command(parser, &((*ast)->right));
        if (state != PARSER_OK)
            return state;
    }
    return state;
}

static enum parser_state parse_list(struct parser *parser, struct ast **ast)
{
    enum parser_state state = parse_command(parser, ast);
    if (state != PARSER_OK)
        return state;

    struct ast *cur = *ast;
    while (1)
    {
        struct ast **tmp = &(cur->left);
        struct token *tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;
        if (tok->type != TOKEN_SEMIC)
            break;
        lexer_pop(parser->lexer);
        token_free(tok);
        state = parse_command(parser, tmp);
        if (state == PARSER_ABSENT)
            break;
        else if (state == PARSER_PANIC)
            return state;
        cur = cur->left;
    }
    return state;
}

static enum parser_state parse_input(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);

    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;
    if (tok->type == TOKEN_EOF)
        return PARSER_OK;

    if (tok->type == TOKEN_NEWL)
    {
        lexer_pop(parser->lexer);
        token_free(tok);
        return parse_input(parser, ast);
    }

    enum parser_state state = PARSER_PANIC;
    if (*ast
        && ((*ast)->type == AST_ROOT || (*ast)->type == AST_PIPE
            || (*ast)->type == AST_AND || (*ast)->type == AST_OR))
        state = parse_list(parser, &((*ast)->right));
    else
        state = parse_list(parser, ast);

    if (state != PARSER_OK)
        return state;

    tok = lexer_peek(parser->lexer);

    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;
    if (tok->type == TOKEN_EOF)
        return PARSER_OK;

    if (tok->type == TOKEN_NEWL || tok->type == TOKEN_PIPE
        || tok->type == TOKEN_AND || tok->type == TOKEN_OR)
    {
        struct ast *placeholder;
        if (tok->type == TOKEN_NEWL)
            placeholder = create_ast(AST_ROOT);
        else if (tok->type == TOKEN_PIPE)
            placeholder = create_ast(AST_PIPE);
        else if (tok->type == TOKEN_AND)
            placeholder = create_ast(AST_AND);
        else if (tok->type == TOKEN_OR)
            placeholder = create_ast(AST_OR);
        else
            errx(1, "never happens");

        placeholder->left = *ast;
        ast = &placeholder;
        parser->ast = (*ast);
        lexer_pop(parser->lexer);
        token_free(tok);
        return parse_input(parser, ast);
    }

    return PARSER_PANIC;
}

enum parser_state parsing(struct parser *parser)
{
    enum parser_state state = parse_input(parser, &(parser->ast));
    if (state != PARSER_OK)
        return handle_parse_error(state, parser);

    if (lexer_peek(parser->lexer)->type == TOKEN_EOF)
        return PARSER_OK;

    return handle_parse_error(PARSER_PANIC, parser);
}

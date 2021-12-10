#include "parser.h"

#include <ast/ast.h>
#include <err.h>
#include <lexer/lexer.h>
#include <lexer/token.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <utils/utils.h>
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
    size_t i = 0;
    while (tok->value[i] != '\0' && is_redirchar(tok->value[i]))
        i++;
    if (i > 2 || tok->value[i] == '\0')
        return PARSER_PANIC;
    struct ast *placeholder = create_ast(AST_REDIR);
    placeholder->val = vec_init();
    placeholder->val->data = strdup(tok->value);
    placeholder->val->size = strlen(tok->value);
    placeholder->val->capacity = strlen(tok->value) + 1;
    if ((*ast)->type == AST_REDIR)
    {
        struct ast *tmp = *ast;
        while (tmp->right)
            tmp = tmp->right;
        placeholder->left = (*ast)->left;
        (*ast)->left = NULL;
        tmp->right = placeholder;
    }
    else
    {
        placeholder->left = *ast;
        *ast = placeholder;
    }
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
    int in_echo = 0;
    if ((*ast)->val)
    {
        int index = 0;
        char *cmd = getcmdname((*ast)->val->data, &index);

        if (strcmp(cmd, "echo") == 0 && stop_echo(tok->type))
            in_echo = 1;
        free(cmd);
    }
    if (tok->type == TOKEN_EXPORT)
    {
        while (tok->type == TOKEN_WORD || tok->type == TOKEN_SEMIC
               || tok->type == TOKEN_EXPORT)
        {
            if (tok->type != TOKEN_SEMIC)
            {
                struct vec *tmp = vec_init();
                tmp->data = strdup(tok->value);
                tmp->size = strlen(tok->value);
                tmp->capacity = tmp->size + 1;
                vec_push(tmp, ' ');
                (*ast)->val = vec_concat((*ast)->val, tmp);
                vec_destroy(tmp);
                free(tmp);
            }

            tok = lexer_pop(parser->lexer);
            token_free(tok);
            tok = lexer_peek(parser->lexer);

            if (tok->type == TOKEN_ERROR)
                return PARSER_PANIC;
        }
        return PARSER_OK;
    }
    if ((tok->type == TOKEN_WORD || tok->type == TOKEN_ECHO
         || tok->type == TOKEN_EXIT || tok->type == TOKEN_DOT || in_echo)
        && tok->type != TOKEN_REDIR)
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
        if (tok->value == NULL || strlen(tok->value) == 0)
            return PARSER_OK;
        if (stop_echo(tok->type))
        {
            (*ast)->val->size--;
            vec_push((*ast)->val, ' ');
            vec_push((*ast)->val, '\0');
        }
        if (strncmp((*ast)->val->data, "break", 5) == 0)
        {
            if (!is_valid_bc((*ast)->val->data + 5))
                return PARSER_PANIC;
            (*ast)->type = AST_BREAK;
        }
        if (strncmp((*ast)->val->data, "continue", 8) == 0)
        {
            if (!is_valid_bc((*ast)->val->data + 8))
                return PARSER_PANIC;
            (*ast)->type = AST_CONTINUE;
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
        ast_free(new);
        return PARSER_PANIC;
    }
    int neg = 0;
    if (tok_type == TOKEN_NEG)
    {
        neg = 1;
        struct token *tok = lexer_pop(parser->lexer);
        token_free(tok);
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
            if (neg)
                return PARSER_PANIC;
            return PARSER_ABSENT;
        }
        if (state == PARSER_ABSENT)
            break;
        element_count++;
    }
    if (element_count == 0 && prefix_count == 0)
    {
        ast_free(new);
        if (neg)
            return PARSER_PANIC;
        return PARSER_ABSENT;
    }
    *ast = new;
    if (neg)
    {
        struct ast *ast_neg = create_ast(AST_NEG);
        ast_neg->left = *ast;
        *ast = ast_neg;
    }
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
    tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_SEMIC && tok->type != TOKEN_NEWL)
        return PARSER_PANIC;

    while (42)
    {
        struct ast *root = create_ast(AST_ROOT);
        root->left = *ast;
        *ast = root;
        tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
            return PARSER_PANIC;
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
            return PARSER_PANIC;

        state = parse_and_or(parser, &(root->right));
        if (state == PARSER_ABSENT)
            break;
        else if (state == PARSER_PANIC)
            return state;
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
    struct ast *new = create_ast(AST_IF);
    *ast = new;

    // getting condition for if
    enum parser_state state = parse_compound_list(parser, &((*ast)->cond));
    if (state != PARSER_OK)
        return state;

    // checking for then token
    if (lexer_peek(parser->lexer)->type != TOKEN_THEN)
        return PARSER_PANIC;

    struct token *tok = lexer_pop(parser->lexer);
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

static enum parser_state parse_do_group(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_DO)
        return PARSER_PANIC;
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    enum parser_state state = parse_compound_list(parser, ast);
    if (state != PARSER_OK)
        return state;
    tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_DONE)
        return PARSER_PANIC;
    tok = lexer_pop(parser->lexer);
    token_free(tok);
    return PARSER_OK;
}

static enum parser_state parse_rule_for(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_WORD)
        return PARSER_PANIC;
    struct ast *for_node = create_ast(AST_FOR);
    tok = lexer_pop(parser->lexer);
    for_node->val = vec_init();
    for_node->val->data = zalloc(sizeof(char) * strlen(tok->value) + 2);
    sprintf(for_node->val->data, "$%s", tok->value);
    for_node->val->size = strlen(tok->value) + 1;
    for_node->val->capacity = strlen(tok->value) + 2;
    token_free(tok);
    tok = lexer_peek(parser->lexer);
    if (tok->type == TOKEN_ERROR)
    {
        ast_free(for_node);
        return PARSER_PANIC;
    }
    if (tok->type == TOKEN_SEMIC)
    {
        add_to_list(for_node, "$@");
        lexer_pop(parser->lexer);
        token_free(tok);
    }
    else
    {
        while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
        {
            lexer_pop(parser->lexer);
            token_free(tok);
        }
        if (tok->type == TOKEN_ERROR)
        {
            ast_free(for_node);
            return PARSER_PANIC;
        }
        tok = lexer_peek(parser->lexer);
        if (tok->type != TOKEN_IN)
        {
            ast_free(for_node);
            return PARSER_PANIC;
        }
        lexer_pop(parser->lexer);
        token_free(tok);
        while ((tok = lexer_peek(parser->lexer))->type == TOKEN_WORD
               || tok->type == TOKEN_ECHO)
        {
            add_to_list(for_node, tok->value);
            tok = lexer_pop(parser->lexer);
            token_free(tok);
        }
        if (tok->type == TOKEN_ERROR)
        {
            ast_free(for_node);
            return PARSER_PANIC;
        }
        if (tok->type != TOKEN_SEMIC && tok->type != TOKEN_NEWL)
        {
            ast_free(for_node);
            return PARSER_PANIC;
        }
        lexer_pop(parser->lexer);
        token_free(tok);
    }
    while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
    {
        lexer_pop(parser->lexer);
        token_free(tok);
    }
    if (tok->type == TOKEN_ERROR)
    {
        ast_free(for_node);
        return PARSER_PANIC;
    }
    (*ast) = for_node;
    return parse_do_group(parser, &((*ast)->left));
}

static enum parser_state parse_rule_while(struct parser *parser,
                                          struct ast **ast)
{
    struct ast *while_node = create_ast(AST_WHILE);
    enum parser_state state = parse_compound_list(parser, &(while_node->cond));
    if (state == PARSER_PANIC)
    {
        ast_free(while_node);
        return state;
    }
    *ast = while_node;
    return parse_do_group(parser, &((*ast)->left));
}

static enum parser_state parse_rule_until(struct parser *parser,
                                          struct ast **ast)
{
    struct ast *until_node = create_ast(AST_UNTIL);
    enum parser_state state = parse_compound_list(parser, &(until_node->cond));
    if (state == PARSER_PANIC)
    {
        ast_free(until_node);
        return state;
    }
    *ast = until_node;
    return parse_do_group(parser, &((*ast)->left));
}

static enum parser_state parse_subshells(struct parser *parser,
                                         struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_OPEN_PAR)
        return PARSER_PANIC;
    lexer_pop(parser->lexer); // Skip '('
    token_free(tok);
    tok = lexer_peek(parser->lexer);
    struct ast *subs = create_ast(AST_SUBSHELL);
    struct vec *vec = vec_init();
    int in_subsubshell = 0;
    while (tok->type != TOKEN_EOF
           && (tok->type != TOKEN_CLOSE_PAR || in_subsubshell))
    {
        if (tok->type == TOKEN_OPEN_PAR)
            in_subsubshell = 1;
        if (tok->type == TOKEN_CLOSE_PAR && in_subsubshell)
            in_subsubshell = 0;
        for (size_t i = 0; i < strlen(tok->value); i++)
            vec_push(vec, tok->value[i]);
        vec_push(vec, ' ');
        token_free(tok);
        lexer_pop(parser->lexer);
        tok = lexer_peek(parser->lexer);
    }
    if (tok->type == TOKEN_CLOSE_PAR)
        vec_push(vec, ')');
    else if (tok->type == TOKEN_EOF)
        vec_push(vec, '\0');
    subs->val = vec;
    subs->left = (*ast);
    (*ast) = subs;
    tok = lexer_pop(parser->lexer); // skip ')'
    token_free(tok);
    return PARSER_OK;
}

static enum parser_state parse_cmdblocks(struct parser *parser,
                                         struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_OPEN_BRAC)
        return PARSER_PANIC;
    lexer_pop(parser->lexer); // Skip '{'
    token_free(tok);
    tok = lexer_peek(parser->lexer);
    struct ast *subs = create_ast(AST_CMDBLOCK);
    struct vec *vec = vec_init();
    int in_subbracket = 0;
    enum token_type tmp_tok = tok->type;
    while (tok->type != TOKEN_EOF
           && (tok->type != TOKEN_CLOSE_BRAC || in_subbracket))
    {
        if (tok->type == TOKEN_OPEN_BRAC)
            in_subbracket = 1;
        if (tok->type == TOKEN_CLOSE_BRAC && in_subbracket)
            in_subbracket = 0;
        for (size_t i = 0; i < strlen(tok->value); i++)
            vec_push(vec, tok->value[i]);
        vec_push(vec, ' ');
        tmp_tok = tok->type;
        token_free(tok);
        lexer_pop(parser->lexer);
        tok = lexer_peek(parser->lexer);
    }
    if (tmp_tok != TOKEN_SEMIC && tmp_tok != TOKEN_NEWL)
    {
        token_free(tok);
        lexer_pop(parser->lexer); // skip ')'
        ast_free(subs);
        vec_destroy(vec);
        free(vec);
        return PARSER_PANIC;
    }
    if (tok->type == TOKEN_CLOSE_BRAC)
        vec_push(vec, '}');
    else if (tok->type == TOKEN_EOF)
        vec_push(vec, '\0');
    subs->val = vec;
    subs->left = (*ast);
    (*ast) = subs;
    tok = lexer_pop(parser->lexer); // skip ')'
    token_free(tok);
    return PARSER_OK;
}

static enum parser_state parse_shell_command(struct parser *parser,
                                             struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type == TOKEN_OPEN_PAR)
    {
        enum parser_state state = parse_subshells(parser, ast);
        if (state != PARSER_OK)
            return PARSER_PANIC;
        else
            return PARSER_OK;
    }
    if (tok->type == TOKEN_OPEN_BRAC)
    {
        enum parser_state state = parse_cmdblocks(parser, ast);
        if (state != PARSER_OK)
            return PARSER_PANIC;
        else
            return PARSER_OK;
    }
    if (tok->type == TOKEN_FOR)
    {
        tok = lexer_pop(parser->lexer);
        token_free(tok);
        return parse_rule_for(parser, ast);
    }
    if (tok->type == TOKEN_WHILE)
    {
        tok = lexer_pop(parser->lexer);
        token_free(tok);
        return parse_rule_while(parser, ast);
    }
    if (tok->type == TOKEN_UNTIL)
    {
        tok = lexer_pop(parser->lexer);
        token_free(tok);
        return parse_rule_until(parser, ast);
    }
    if (tok->type == TOKEN_IF)
    {
        tok = lexer_pop(parser->lexer);
        token_free(tok);
        return parse_rule_if(parser, ast);
    }
    return PARSER_PANIC;
}
/*
static enum parser_state parse_funcdec(struct parser *parser, struct ast **ast)
{
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_WORD)
        return PARSER_ABSENT;
    struct ast *fun_node = create_ast(AST_FUNCTION);
    struct vec *vec = vec_init();
    vec->data = strdup(tok->value);
    token_free(tok);
    lexer_pop(parser->lexer);
    tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_OPEN_PAR)
    {
        ast_free(fun_node);
        vec_destroy(vec);
        free(vec);
        return PARSER_ABSENT;
    }
    // Skip '('
    token_free(tok);
    lexer_pop(parser->lexer);
    tok = lexer_peek(parser->lexer);
    if (tok->type != TOKEN_CLOSE_PAR)
    {
        ast_free(fun_node);
        vec_destroy(vec);
        free(vec);
        return PARSER_ABSENT;
    }
    // Skip ')'
    token_free(tok);
    lexer_pop(parser->lexer);
    tok = lexer_peek(parser->lexer);
    // Skip '\n'
    while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
    {
        lexer_pop(parser->lexer);
        token_free(tok);
    }
    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;
    fun_node->val = vec;
    (*ast) = fun_node;
    return parse_shell_command(parser, &((*ast)->left));
}
*/
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
    struct token *tok = lexer_peek(parser->lexer);
    if (tok->type == TOKEN_ERROR)
        return PARSER_PANIC;
    int neg = 0;
    if (tok->type == TOKEN_NEG)
    {
        neg = 1;
        tok = lexer_pop(parser->lexer);
        token_free(tok);
    }

    // parsing command
    enum parser_state state = parse_command(parser, ast);
    if (state != PARSER_OK)
        return state;
    // parsing ( '|' ( /n )* command )*
    while (1)
    {
        // parsing '|'
        tok = lexer_peek(parser->lexer);
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
            return PARSER_PANIC;
    }
    if (neg)
    {
        struct ast *ast_neg = create_ast(AST_NEG);
        ast_neg->left = *ast;
        *ast = ast_neg;
    }
    return state;
}

static enum parser_state parse_list(struct parser *parser, struct ast **ast)
{
    enum parser_state state = parse_command(parser, ast);
    if (state != PARSER_OK)
        return state;

    while (1)
    {
        struct ast *root = create_ast(AST_ROOT);
        root->left = *ast;
        struct token *tok = lexer_peek(parser->lexer);
        if (tok->type == TOKEN_ERROR)
        {
            ast_free(root);
            return PARSER_PANIC;
        }
        if (tok->type != TOKEN_SEMIC)
        {
            *ast = root;
            break;
        }
        lexer_pop(parser->lexer);
        token_free(tok);
        state = parse_command(parser, &(root->right));
        if (state == PARSER_ABSENT)
        {
            state = PARSER_OK;
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
    return state;
}

int should_have_next(enum token_type type)
{
    return type == TOKEN_PIPE || type == TOKEN_AND || type == TOKEN_OR;
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
            || (*ast)->type == AST_AND || (*ast)->type == AST_OR
            || (*ast)->type == AST_REDIR))
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
        || tok->type == TOKEN_AND || tok->type == TOKEN_OR
        || tok->type == TOKEN_REDIR)
    {
        struct ast *placeholder;
        if (tok->type == TOKEN_NEWL || tok->type == TOKEN_REDIR)
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
        enum token_type last_tok = tok->type;
        lexer_pop(parser->lexer);
        token_free(tok);
        if (should_have_next(last_tok))
        {
            while ((tok = lexer_peek(parser->lexer))->type == TOKEN_NEWL)
            {
                lexer_pop(parser->lexer);
                token_free(tok);
            }
            if (tok->type == TOKEN_ERROR || tok->type == TOKEN_EOF)
                return PARSER_PANIC;
        }
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

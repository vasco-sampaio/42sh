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

static int is_valid(char *str)
{
    size_t len = strlen(str);
    if (str[len - 1] == '}')
    {
        str[len - 1] = '\0'; // Remove closing parenthesis
        return 1;
    }
    return 0;
}

int cmdblock(char *args)
{
    if (!is_valid(args))
    {
        fprintf(stderr, "42sh: Syntax error: end of file unexecpected\n");
        return 2;
    }
    struct parser *parser = create_parser();
    parser->lexer = lexer_create(args);
    enum parser_state state = parsing(parser);
    int return_code = 0;
    if (state != PARSER_OK)
        return_code = 2;
    return_code = ast_eval(parser->ast, &return_code);
    parser_free(parser);
    return return_code;
}

int add_function(struct ast *ast)
{
    printf("ADD");
    ast->val->data--;
    return 0;
}
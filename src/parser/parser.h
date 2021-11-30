#ifndef PARSER_H
#define PARSER_H

#include <ast/ast.h>
#include <lexer/lexer.h>

enum parser_state
{
    PARSER_OK,
    PARSER_PANIC,
    PARSER_ABSENT
};

struct parser
{
    struct ast *ast;
    struct lexer *lexer;
};

enum parser_state parsing(struct parser *parser);

struct parser *create_parser();

void parser_free(struct parser *parser);

#endif /* ! PARSER_H */

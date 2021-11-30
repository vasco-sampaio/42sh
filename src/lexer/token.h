#ifndef TOKEN_H
#define TOKEN_H

#include <utils/vec.h>

/**
 * \brief possible types for a token
 * All commands are after TOKEN_ECHO
 */
enum token_type
{
    TOKEN_IF = 0,
    TOKEN_THEN = 1,
    TOKEN_ELIF = 2,
    TOKEN_ELSE = 3,
    TOKEN_FI = 4,
    TOKEN_SEMIC = 5,
    TOKEN_NEWL = 6,
    TOKEN_WORD = 7,
    TOKEN_EOF = 8,
    TOKEN_REDIR = 9,
    TOKEN_NEG = 10,
    TOKEN_OR = 11,
    TOKEN_AND = 12,
    TOKEN_PIPE = 13,
    TOKEN_ECHO = 14,
    TOKEN_ERROR = 15
};

/**
 * \brief Structure for a token
 */
struct token
{
    enum token_type type;
    char *value;
};

// Create a token according to type
struct token *token_create(enum token_type type);

void token_free(struct token *token);

#endif /* ! TOKEN_H */

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
    TOKEN_WHILE = 14,
    TOKEN_FOR = 15,
    TOKEN_UNTIL = 16,
    TOKEN_DO = 17,
    TOKEN_DONE = 18,
    TOKEN_IN = 19,
    TOKEN_OPEN_PAR = 20,
    TOKEN_CLOSE_PAR = 21,
    TOKEN_OPEN_BRAC = 22,
    TOKEN_CLOSE_BRAC = 23,
    TOKEN_BQUOTE = 24,
    TOKEN_CASE = 25,
    TOKEN_ESAC = 26,
    TOKEN_DSEMIC = 27,
    TOKEN_ECHO = 28,
    TOKEN_EXIT = 29,
    TOKEN_EXPORT = 30,
    TOKEN_DOT = 31,
    TOKEN_ERROR = 32
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

/**
 * \brief Duplicate a token
 */
struct token *token_dup(struct token *tok);

#endif /* ! TOKEN_H */

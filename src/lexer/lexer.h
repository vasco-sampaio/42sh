#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

#include "token.h"

/**
 * \brief Stucture for lexer.
 */
struct lexer
{
    char *input;
    size_t pos;
    struct token *current_tok;
};

/**
 * \brief Create and initialize a new lexer
 * The new lexer is initilialized as follow:
 * - input: the input string
 * - state: DEFAULT
 * - pos: 0
 * - current_tok: the first token
 * */
struct lexer *lexer_create(char *input);

/**
 * \brief Free the lexer structure.
 * */
void lexer_free(struct lexer *lexer);

/**
 * \brief Return the current token without going forward in the input.
 * Multiple calls to lexer_peek will return the same token.
 * */
struct token *lexer_peek(struct lexer *lexer);

/**
 * \brief Return the current token and go forward in the input.
 * Save the next token in lexer->current_tok
 * */
struct token *lexer_pop(struct lexer *lexer);

#endif /* ! LEXER_H */

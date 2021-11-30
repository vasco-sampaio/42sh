#include "token.h"

#include <utils/alloc.h>

struct token *token_create(enum token_type type)
{
    struct token *new = zalloc(sizeof(struct token));
    new->type = type;
    new->value = NULL;
    return new;
}

void token_free(struct token *token)
{
    if (token->value != NULL)
        free(token->value);
    free(token);
}

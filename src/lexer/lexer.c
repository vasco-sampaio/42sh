#include "lexer.h"

#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <utils/alloc.h>
#include <utils/utils.h>
#include <utils/vec.h>

#define SIZE 12

static int isvalidampersand(char *str)
{
    int i = 0;
    while (str[i] != '<' && str[i] != '>')
        i++;
    if (str[i + 1] != 0 && str[i + 2] != 0 && str[i + 3] != 0)
    {
        if (str[i + 1] == '&' && isdigit(str[i + 2]) && str[i + 3] != ' ')
            return 0;
    }
    return 1;
}

static int isvalidredir(char *str)
{
    int i = 0;
    while (str[i] != 0 && str[i] != ' ')
        i++;
    if (str[i] == 0)
        return isvalidampersand(str);
    if (str[i + 1] != 0 && str[i + 1] == '&')
        return 0;
    return isvalidampersand(str);
}

static int is_redir(char *str)
{
    size_t i = 0;
    int quotes = 0;
    while (str[i] != '\0')
    {
        if (str[i] == '\'')
            quotes++;
        if (str[i] == '<' || str[i] == '>')
        {
            if (quotes == 1)
                return 0;
            if (!isvalidredir(str + i))
                return -1;
            return 1;
        }
        i++;
    }
    return 0;
}

static int match_token(char *str, int quote)
{
    int res = is_redir(str);
    if (res == 1)
        return TOKEN_REDIR;
    if (res == -1)
    {
        fprintf(stderr, "Syntax error: '&' unexpected\n");
        return TOKEN_ERROR;
    }
    char *names[SIZE] = { "if", "then", "else", "elif", "fi", ";",
                          "\n", "!",    "||",   "&&",   "|",  "echo" };
    int types[SIZE] = { TOKEN_IF, TOKEN_THEN,  TOKEN_ELSE, TOKEN_ELIF,
                        TOKEN_FI, TOKEN_SEMIC, TOKEN_NEWL, TOKEN_NEG,
                        TOKEN_OR, TOKEN_AND,   TOKEN_PIPE, TOKEN_ECHO };
    for (size_t i = 0; i < SIZE; i++)
    {
        if (strcmp(str, names[i]) == 0)
        {
            if (quote && types[i] < TOKEN_ECHO)
                break;
            return types[i];
        }
    }
    return TOKEN_WORD;
}

/**
 * \brief: Fill the vector with the content between quotes.
 * If no matching quotes find, throw an warning
 * @return value: -1 in case of lexing error
 *                 0 otherwise
 */
static int handle_quotes(struct lexer *lexer, struct vec *vec, size_t len)
{
    // vec_push(vec, lexer->input[lexer->pos++]); // Skip opening quote
    lexer->pos++;
    while (lexer->pos < len && lexer->input[lexer->pos] != '\'')
        vec_push(vec, lexer->input[lexer->pos++]);
    if (lexer->pos == len)
    {
        fprintf(stderr, "Syntax error: Unterminated quoted string\n");
        return -1;
    }

    // vec_push(vec, lexer->input[lexer->pos++]); // Skip closing quote
    lexer->pos++;
    return 0;
}

static size_t get_redir_idx(struct lexer *lexer, size_t len)
{
    size_t i = lexer->pos;
    int quote = 0;
    while (i < len && lexer->input[i] != '>' && lexer->input[i] != '<')
    {
        if (lexer->input[i] == '\'')
            quote++;
        i++;
    }
    if (quote == 1 && i != len) // Quoted '<' '>'
        return len;
    if (i == len)
        return len;
    if (i == 0)
        return 0;

    if (isdigit(lexer->input[i - 1])
        && (i - 2 < 0 || lexer->input[i - 2] == ' '))
        return i - 1;
    return i;
}

/**
 * \brief: Return the substring in the input of the lexer.
 * A substring is eneded by a separator.
 * @param len: lenght of lexer->input
 * @return value: return wether the lexed word is quoted.
 *                -1 if lexing error
 */
static int get_substr(struct lexer *lexer, struct vec *vec, size_t len)
{
    size_t before = lexer->pos;
    int quote = 0;
    size_t redir_index = get_redir_idx(lexer, len);
    if (redir_index == lexer->pos)
        redir_index = len;
    while (lexer->pos < len && !is_separator(lexer->input[lexer->pos])
           && lexer->pos < redir_index)
    {
        char current = lexer->input[lexer->pos];
        if (current == '\'')
        {
            quote = 1;
            int error = handle_quotes(lexer, vec, len);
            if (error == -1)
                return -1;
        }
        else
        {
            vec_push(vec, current);
            if ((current == '<' || current == '>')
                && lexer->input[lexer->pos + 1] == ' ')
            {
                vec_push(vec, lexer->input[lexer->pos + 1]);
                lexer->pos++;
            }
            lexer->pos++;
        }
    }
    // check if the first character was a separator and different of space
    if (lexer->pos == before && lexer->input[before] != ' ')
    {
        if (lexer->input[lexer->pos] != '<' && lexer->input[lexer->pos] != '>')
        {
            char c = lexer->input[lexer->pos++];
            vec_push(vec, c);
            if (lexer->pos < len && lexer->input[lexer->pos] == c)
                vec_push(vec, lexer->input[lexer->pos++]);
        }
    }
    return quote;
}

/**
 * \brief: Return a lexed a token in input.
 * Fill the value of the token.
 */
struct token *get_token(struct lexer *lexer)
{
    size_t input_len = strlen(lexer->input);
    while (lexer->pos < input_len && lexer->input[lexer->pos] == ' ')
        lexer->pos++;
    if (lexer->pos >= input_len)
        return token_create(TOKEN_EOF);
    struct vec *vec = vec_init();
    int quote = get_substr(lexer, vec, input_len);
    struct token *tok = NULL;
    if (quote == -1)
        tok = token_create(TOKEN_ERROR);
    else
    {
        char *sub_str = vec_cstring(vec);
        tok = token_create(match_token(sub_str, quote));
        tok->value = strdup(sub_str);
    }
    vec_destroy(vec);
    free(vec);
    return tok;
}

struct lexer *lexer_create(char *input)
{
    struct lexer *new = zalloc(sizeof(struct lexer));
    new->input = input;
    new->pos = 0;
    new->current_tok = get_token(new);
    return new;
}

void lexer_free(struct lexer *lexer)
{
    if (lexer->current_tok != NULL)
        token_free(lexer->current_tok);
    free(lexer);
}

struct token *lexer_peek(struct lexer *lexer)
{
    return lexer->current_tok;
}

struct token *lexer_pop(struct lexer *lexer)
{
    struct token *current = lexer->current_tok;
    lexer->current_tok = get_token(lexer);
    return current;
}

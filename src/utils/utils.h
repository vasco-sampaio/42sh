#ifndef UTILS_H
#define UTILS_H

#include <lexer/token.h>

/**
 * \brief Options structure
 * @param p: activate the pretty-priting of the ast
 * @param c: use 42sh with a string
 * @param input: the input string
 */
struct opts
{
    int p;
    int c;
    int optind;
    char *input;
};

int not_as_escape(char *str, int pos);

/**
 * \brief Return if the char c is a separator
 */
int is_separator(char c);

int is_redirchar(char c);

/**
 * \brief Get the command name from a string.
 * @param cmd: the string containing the command
 * @param i: the index where the commands stops
 * @return: the command, allocated
 */
char *getcmdname(char *cmd, int *i);

/**
 * \brief Return if the token should be printed by echo
 */
int stop_echo(enum token_type type);

/**
 * \brief Return if str is a number among spaces
 */
int is_valid_bc(char *str);

#endif /* ! UTILS_H */

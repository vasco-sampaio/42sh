#include "utils.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

/**
 * \brief: Return wether the sequence of \ escape itself
 */
int not_as_escape(char *str, int pos)
{
    int count = 0;
    while (pos >= 0 && str[pos] == '\\')
    {
        count++;
        pos--;
    }
    return count % 2 == 0;
}

int is_separator(char c)
{
    char separator[7] = ";|\0\n\t "; // Array of possible separator
    for (size_t i = 0; i < 7; i++)
    {
        if (c == separator[i])
            return 1;
    }
    return 0;
}

// Return wether the char is a redir mode
int is_redirchar(char c)
{
    char redir_char[4] = "<>|&";
    for (size_t i = 0; i < 4; i++)
    {
        if (c == redir_char[i])
            return 1;
    }
    return 0;
}

char *getcmdname(char *cmd, int *i)
{
    while (cmd[*i] != '\0' && !is_separator(cmd[*i]))
        (*i)++;
    return strndup(cmd, *i);
}

int stop_echo(enum token_type type)
{
    if (type != TOKEN_EOF && type != TOKEN_SEMIC && type != TOKEN_NEWL
        && type != TOKEN_PIPE && type != TOKEN_AND && type != TOKEN_OR
        && type != TOKEN_REDIR && type != TOKEN_DSEMIC)
        return 1;
    return 0;
}

int is_valid_bc(char *str)
{
    int i = 0;
    while (str[i] != 0 && isblank(str[i]))
        i++;
    if (str[i] == 0)
        return 1;
    while (str[i] != 0 && isdigit(str[i]))
        i++;
    return str[i] == 0;
}

void cas_free(struct cas *cas)
{
    while (cas)
    {
        struct cas *tmp = cas->next;
        free(cas->pattern);
        ast_free(cas->ast);
        free(cas);
        cas = tmp;
    }
}

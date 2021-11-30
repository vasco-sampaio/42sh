#ifndef UTILS_H
#define UTILS_H

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

/**
 * \brief Return if the char c is a separator
 */
int is_separator(char c);

#endif /* ! UTILS_H */

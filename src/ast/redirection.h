#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "ast.h"

/**
 * \brief Simple left redirection
 * @details redirection '>'
 */
int redir_simple_left(struct ast *left, int fd, char *right);

/**
 * \brief Simple right redirection
 * @details redirection '<'
 */
int redir_simple_right(struct ast *left, int fd, char *right);

/**
 * \brief Double left redirection
 * @details redirection '>>'
 */
int redir_double_left(struct ast *left, int fd, char *right);

/**
 * \brief ampersand left redirection
 * @details redirection '>&'
 */
int redir_ampersand_left(struct ast *left, int fd, char *right);

/**
 * \brief ampersand right redirection
 * @details redirection '&<'
 */
int redir_ampersand_right(struct ast *left, int fd, char *right);

/**
 * \brief left and right redirection
 * @details redirection '<>'
 */
int redir_left_right(struct ast *left, int fd, char *right);

#endif /* ! REDIRECTION_H  */

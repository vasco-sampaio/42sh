#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/utils.h>

#include "ast.h"

#define NONE 0
#define SIMPLE 1
#define DOUBLE 2

struct list *vars;

char *my_strstr(char *str, char *var)
{
    int context = NONE;
    for (int i = 0; str[i] != 0; i++)
    {
        if (str[i] == '\"')
        {
            if (context == NONE)
                context = DOUBLE;
            else if (context == DOUBLE)
                context = NONE;
        }
        if (str[i] == '\'')
        {
            if (context == NONE)
                context = SIMPLE;
            else if (context == SIMPLE)
                context = NONE;
            continue;
        }
        if (context == SIMPLE)
            continue;
        if (str[i] == var[0])
        {
            int j = 0;
            for (; var[j] != 0 && str[i + j] != 0; j++)
            {
                if (var[j] != str[i + j])
                    break;
            }
            if (var[j] == 0)
            {
                int n = i + j;
                if (str[n] == 0 || !isalnum(str[n]) || str[n - 1] == '}')
                    return str + i;
            }
        }
    }
    return NULL;
}

char *replace_vars(char *str, char *var, char *replace)
{
    if (var == NULL)
        return strdup(str);
    char *substring = NULL;
    size_t to_copy = 0;
    char *cmd = strdup(str);
    while ((substring = my_strstr(cmd, var)) != NULL)
    {
        to_copy = substring - cmd; // determine length before substring
        char *before = strndup(cmd, to_copy);
        char *after = strdup(substring + strlen(var));
        char *tmp =
            zalloc(sizeof(char)
                   * (strlen(before) + strlen(replace) + strlen(after) + 1));
        sprintf(tmp, "%s%s%s", before, replace, after);
        free(cmd);
        free(before);
        free(after);
        cmd = tmp;
    }
    return cmd;
}

int is_var_sep(char c)
{
    if (is_separator(c))
        return 1;
    return c == '$' || c == '=' || c == '\'' || c == '\"';
}

static char *replace_at_by(char *str, int status, int len, char *replace)
{
    char *new = zalloc(sizeof(char) * (strlen(str) + strlen(replace) + 1));
    int spaces = 0;
    if (strcmp(replace, "") == 0
        && (str[status + len] == ' ' || str[status + len] == 0))
        spaces++;
    if (strncmp(str + status + 1, "@", 1) == 0)
    {
        if (status > 0 && str[status - 1] == '\"' && str[status + len] == '\"')
            spaces++;
        if (status > 1 && str[status - 2] == ' ')
            spaces++;
    }
    char *before = strndup(str, status - spaces);
    char *after = strdup(str + status + len);
    sprintf(new, "%s%s%s", before, replace, after);
    free(str);
    free(before);
    free(after);
    return new;
}

static char *sub_replace(char *str, int status, int *i, int brackets, char *var,
                         char *var_rep)
{
    char *name =
        strndup(str + status + 1 + brackets, *i - status - brackets * 2 - 1);
    struct list *cur = vars;
    char *replace = "";
    while (cur)
    {
        if (strcmp(cur->name, name) == 0)
        {
            replace = cur->value;
            break;
        }
        cur = cur->next;
    }
    if (var != NULL && strcmp(name, var + 1) == 0)
        replace = var_rep;
    str = replace_at_by(str, status, *i - status, replace);
    *i = status;
    if (*i > 0)
        (*i)--;
    free(name);
    return str;
}

char *expand_vars(char *str, char *var, char *var_rep)
{
    int i = 0;
    int status = -1;
    int brackets = 0;
    int context = NONE;
    while (str[i] != 0)
    {
        if (str[i] == '\'')
        {
            if (context == NONE)
                context = SIMPLE;
            else if (context == SIMPLE)
                context = NONE;
        }
        if (str[i] == '\"')
        {
            if (context == NONE)
                context = DOUBLE;
            else if (context == DOUBLE)
                context = NONE;
        }
        if (context != SIMPLE && status == -1 && str[i] == '$'
            && (i == 0 || str[i - 1] != '\\') && !is_var_sep(str[i + 1]))
        {
            if (str[i + 1] == '{')
                brackets = 1;
            status = i;
        }
        else if ((is_var_sep(str[i]) || (brackets == 1 && str[i] == '}'))
                 && status != -1)
        {
            if (str[i] == '}' && brackets == 1)
                i++;

            str = sub_replace(str, status, &i, brackets, var, var_rep);

            status = -1;
            brackets = 0;
        }
        i++;
    }
    if (status != -1)
    {
        str = sub_replace(str, status, &i, brackets, var, var_rep);
    }
    return str;
}

char *remove_quotes(char *str)
{
    int i = 0;
    int context = NONE;
    int index = 0;
    char *new = zalloc(sizeof(char) * (strlen(str) + 1));
    while (str[i] != 0)
    {
        if (str[i + 1] != '\0' && str[i] == '\\' && !isspace(str[i + 1]))
        {
            if ((context != DOUBLE || str[i + 1] != '\'') && context != SIMPLE)
            {
                i++;
                continue;
            }
            else if (str[i + 1] == '\'')
            {
                new[index++] = str[i];
                i += 2;
                continue;
            }
        }
        if (str[i] == '\'' && (i == 0 || str[i - 1] != '\\'))
        {
            if (context == NONE)
            {
                context = SIMPLE;
                i++;
                continue;
            }
            if (context == SIMPLE)
            {
                context = NONE;
                i++;
                continue;
            }
        }
        if (str[i] == '\"' && (i == 0 || str[i - 1] != '\\'))
        {
            if (context == NONE)
            {
                context = DOUBLE;
                i++;
                continue;
            }
            if (context == DOUBLE)
            {
                context = NONE;
                i++;
                continue;
            }
        }

        new[index++] = str[i];
        i++;
    }
    free(str);
    return new;
}

void add_var(struct list *new)
{
    if (!vars)
    {
        vars = new;
        return;
    }
    struct list *cur = vars;
    struct list *prev = NULL;
    while (cur && strcmp(cur->name, new->name) != 0)
    {
        prev = cur;
        cur = cur->next;
    }
    if (!cur)
    {
        prev->next = new;
        return;
    }
    if (!prev)
    {
        new->next = cur->next;
        vars = new;
        free_var(cur);
        return;
    }
    new->next = cur->next;
    free_var(cur);
    prev->next = new;
}

int is_var_assign(char *str)
{
    char *equal = strchr(str, '=');
    char *space = strchr(str, ' ');
    char *quote = strchr(str, '\"');
    char *squote = strchr(str, '\'');
    if (equal == str)
        return 0;
    if (isdigit(str[0]) || !equal || (space && space < equal))
        return 0;
    if ((quote && quote < equal) || (squote && squote < equal))
        return 0;
    char *tmp = str;
    while (tmp != equal && isalnum(tmp[0]))
        tmp++;
    if (tmp != equal)
        return 0;

    struct list *var = zalloc(sizeof(struct list));
    var->name = strndup(str, equal - str);
    var->value = strdup(equal + 1);
    add_var(var);
    return 1;
}

void var_assign_special(char *str)
{
    char *equal = strchr(str, '=');
    char *before = strndup(str, equal - str);
    struct list *var = zalloc(sizeof(struct list));
    var->name = before;
    var->value = strdup(equal + 1);
    add_var(var);
}

char *build_var(char *name, char *value)
{
    char *new = zalloc(sizeof(char) * (strlen(name) + 1 + strlen(value) + 1));
    sprintf(new, "%s=%s", name, value);
    return new;
}

char *my_itoa(int n)
{
    char *new = zalloc(sizeof(char) * 20); // more than max int
    sprintf(new, "%d", n);
    return new;
}

void set_special_vars(void)
{
    int pid = getpid();
    char *value = my_itoa(pid);
    char *var = build_var("$", value);
    var_assign_special(var);

    free(var);
    free(value);

    var = build_var("?", "0");
    var_assign_special(var);
    free(var);

    var = build_var("RANDOM", "");
    var_assign_special(var);
    free(var);

    int uid = getuid();
    value = my_itoa(uid);
    var = build_var("UID", value);
    var_assign_special(var);

    free(var);
    free(value);

    var = build_var("IFS", " \t\n");
    var_assign_special(var);
    free(var);

    char *home = getenv("HOME");
    var = build_var("OLDPWD", home);
    var_assign_special(var);
    free(var);
}

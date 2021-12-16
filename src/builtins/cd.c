#include <ast/ast.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/alloc.h>
#include <utils/vec.h>

#include "builtin.h"

static char *looping(void)
{
    int size = 4096;
    char *old = zalloc(size * sizeof(char));
    while (!(old = getcwd(old, size)))
    {
        size *= 2;
        old = xrealloc(old, size);
    }
    return old;
}

int cd(char *args)
{
    char *home = getenv("HOME");
    if (strlen(args) == 0 && home == NULL)
        return 0;
    if (strlen(args) == 0)
        return chdir(home);
    while (*args && args[0] == ' ')
        args++;
    if (!strcmp("-", args))
    {
        char *old_pwd = getenv("OLDPWD");
        printf("%s\n", old_pwd);
        if (chdir(old_pwd) == -1)
        {
            fprintf(stderr, "Can't cd into %s\n", args);
            return 2;
        }
        return 0;
    }
    char *old = looping();

    // update oldpwd in local variables
    char *var = build_var("OLDPWD", old);
    var_assign_special(var);
    free(var);

    if (setenv("OLDPWD", old, 1) == -1)
    {
        free(old);
        return -1;
    }
    free(old);
    int res = chdir(args);
    if (res == -1)
    {
        fprintf(stderr, "Can't cd into %s\n", args);
        return 2;
    }
    old = looping();
    if (setenv("PWD", old, 1) == -1)
    {
        free(old);
        return -1;
    }
    free(old);
    return res;
}

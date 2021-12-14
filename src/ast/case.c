#define _GNU_SOURCE

#include <fnmatch.h>

#include "ast.h"

int handle_case(struct ast *ast)
{
    struct cas *cas = ast->cas;
    while (cas)
    {
        if (fnmatch(cas->pattern, ast->word, FNM_EXTMATCH) == 0)
        {
            int return_code = 0;
            return_code = ast_eval(cas->ast, &return_code);
            return return_code;
        }
        cas = cas->next;
    }
    return 1;
}

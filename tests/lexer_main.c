#include <lexer/lexer.h>
#include <stdio.h>

void print_tokens(char *input)
{
    struct lexer *lexer = lexer_create(input);
    struct token *current = NULL;
    while (1)
    {
        current = lexer_pop(lexer);
        printf("%d", current->type);
        if (current->type == TOKEN_EOF)
            break;
        token_free(current);
    }
    printf("\n");
    token_free(current);
    lexer_free(lexer);
}

int main(int argc, char **argv)
{
    argc--;
    print_tokens(argv[1]);
}

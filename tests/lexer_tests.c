#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <lexer/lexer.h>
#include <lexer/token.h>

void redirect_stdout(void)
{
    cr_redirect_stdout();
}

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
    token_free(current);
    lexer_free(lexer);
}

Test(LexerSuite, ifTest)
{
    redirect_stdout();
    char input[] = "if";
    char expected[] = "08";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, empty)
{
    redirect_stdout();
    char input[] = "";
    char expected[] = "8";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, simpleWord)
{
    redirect_stdout();
    char input[] = "Hello;";
    char expected[] = "758";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, incompleteIf)
{
    redirect_stdout();
    char input[] = "if echo;";
    char expected[] = "01458";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, hardIf)
{
    redirect_stdout();
    char input[] = "if echo test; then \n fi";
    char expected[] = "014751648";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, Squote1)
{
    redirect_stdout();
    char input[] = "'abc'";
    char expected[] = "78";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, SquoteHard)
{
    redirect_stdout();
    char input[] = "'echo'";
    char expected[] = "148";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, wordValue)
{
    char input[] = "lol";
    char expected[] = "lol";
    struct lexer *lexer = lexer_create(input);
    cr_assert_str_eq(expected, lexer->current_tok->value);
    lexer_free(lexer);
}

Test(LexerSuite, echoNonAlphanum)
{
    redirect_stdout();
    char input[] = "echo !!";
    char expected[] = "1478";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, echo_quote)
{
    redirect_stdout();
    char input[] = "echo'lol'";
    char expected[] = "78";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, contextTest)
{
    redirect_stdout();
    char input[] = "echo lol 'ife'";
    char expected[] = "14778";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, echoPlusPlus)
{
    redirect_stdout();
    char input[] = "echo++";
    char expected[] = "78";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, lolQuotes)
{
    redirect_stdout();
    char input[] = "echo 'lo;l'";
    char expected[] = "1478";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, echolol)
{
    redirect_stdout();
    char input[] = "echo lo;l";
    char expected[] = "147578";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, ifQuoted)
{
    redirect_stdout();
    char input[] = "'if'";
    char expected[] = "78";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, echoLolQuoted)
{
    redirect_stdout();
    char input[] = "echo'lol'";
    char expected[] = "echolol";
    struct lexer *lexer = lexer_create(input);
    cr_assert_str_eq(expected, lexer->current_tok->value);
    lexer_free(lexer);
}

Test(LexerSuite, echoCmdQuoted)
{
    redirect_stdout();
    char input[] = "'echo' popo";
    char expected[] = "1478";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, errorToken)
{
    redirect_stdout();
    char input[] = "echo 'not finished quotes";
    char expected[] = "14158";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, basicRedir)
{
    redirect_stdout();
    char input[] = "echo test > /dev/null";
    char expected[] = "14798";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, basicRedirNoSpaces)
{
    redirect_stdout();
    char input[] = "echo test>/dev/null";
    char expected[] = "14798";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, basicRedirInvalidSpace)
{
    redirect_stdout();
    char input[] = "echo test> &2";
    char expected[] = "147158";
    print_tokens(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

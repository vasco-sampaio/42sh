#include <builtins/builtin.h>
#include <criterion/criterion.h>
#include <criterion/redirect.h>

void redirect_stdout(void)
{
    cr_redirect_stdout();
}

Test(LexerSuite, simpleWord)
{
    redirect_stdout();
    char input[] = "Salut";
    echo(input);
    char expected[] = "Salut\n";
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, empty)
{
    redirect_stdout();
    char input[] = "";
    echo(input);
    char expected[] = "\n";
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, sentence)
{
    redirect_stdout();
    char input[] = "Hello World!";
    echo(input);
    char expected[] = "Hello World!\n";
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, nOption)
{
    redirect_stdout();
    char input[] = "-n no newline";
    char expected[] = "no newline";
    echo(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, eOption)
{
    redirect_stdout();
    char input[] = "-e aaaa\\neeeeee";
    char expected[] = "aaaa\neeeeee\n";
    echo(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, no_eOption)
{
    redirect_stdout();
    char input[] = "aaaa\\neeeeee";
    char expected[] = "aaaa\\neeeeee\n";
    echo(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, neOption)
{
    redirect_stdout();
    char input[] = "-ne aaaa\\neeeeee";
    char expected[] = "aaaa\neeeeee";
    echo(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

Test(LexerSuite, enOption)
{
    redirect_stdout();
    char input[] = "-en aaaa\\neeeeee";
    char expected[] = "aaaa\neeeeee";
    echo(input);
    fflush(NULL);
    cr_assert_stdout_eq_str(expected);
}

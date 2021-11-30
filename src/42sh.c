#include <ast/ast.h>
#include <err.h>
#include <getopt.h>
#include <io/cstream.h>
#include <parser/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utils/utils.h>
#include <utils/vec.h>

static struct opts *parse_opts(int argc, char **argv)
{
    struct opts *opts = zalloc(sizeof(struct opts));
    static struct option long_options[] = {
        { "pretty-print", no_argument, NULL, 'p' },
        { "c", required_argument, NULL, 'c' },
        { NULL, 0, NULL, 0 }
    };
    int c;
    while ((c = getopt_long(argc, argv, "p:c:", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'p':
            opts->p = 1;
            break;
        case 'c':
            opts->c = 1;
            opts->input = optarg;
            break;
        case '?':
            fprintf(stderr, "Usage: %s [OPTIONS] [SCRIPTS] [ARGUMENTS ...]\n",
                    argv[0]);
            free(opts);
            return NULL;
        default:
            break;
        }
    }
    opts->optind = optind;
    return opts;
}

/**
 * \brief Parse the command line arguments
 * \return A character stream
 */
static struct cstream *parse_args(int argc, char *argv[], struct opts **opts)
{
    // If launched without argument, read the standard input
    *opts = parse_opts(argc, argv);
    if (!(*opts))
        return NULL;
    if (argc == 1 || (argc == 2 && (*opts)->p))
    {
        if (isatty(STDIN_FILENO))
            return cstream_readline_create();
        return cstream_file_create(stdin, /* fclose_on_free */ false);
    }

    // 42sh FILENAME
    if ((argc == 2 && !(*opts)->p) || (argc == 3 && (*opts)->p))
    {
        FILE *fp = fopen(argv[(*opts)->optind], "r");
        if (fp == NULL)
        {
            warn("failed to open input files");
            return NULL;
        }

        return cstream_file_create(fp, /* fclose_on_free */ true);
    }

    return NULL;
}

/**
 * \brief Read and print lines on newlines until EOF
 * \return An error code
 */
enum error read_print_loop(struct cstream *cs, struct vec *line,
                           struct parser *parser, struct opts *opts)
{
    enum error err;
    struct vec *final = NULL;

    if (!(opts->c))
    {
        while (true)
        {
            // Read the next character
            int c;
            if ((err = cstream_pop(cs, &c)))
                return err;

            // If the end of file was reached, stop right there
            if (c == EOF)
                break;

            // If a newline was met, print the line
            if (c == '\n')
            {
                final = vec_concat(final, line);
                final->size--;
                vec_push(final, '\n');
                vec_push(final, '\0');
                vec_reset(line);
                continue;
            }

            // Otherwise, add the character to the line
            vec_push(line, c);
        }
        vec_destroy(line);
    }
    else
    {
        final = vec_init();
        final->data = strdup(opts->input);
        final->size = strlen(opts->input);
        final->capacity = strlen(opts->input);
    }
    parser->lexer = lexer_create(vec_cstring(final));
    enum parser_state state = parsing(parser);
    if (state != PARSER_OK)
    {
        vec_destroy(final);
        free(final);
        return 2;
    }
    if (opts->p)
        pretty_print(parser->ast);
    int eval = ast_eval(parser->ast);
    vec_destroy(final);
    free(final);
    return eval;
}

int main(int argc, char *argv[])
{
    int rc = 0;

    // Parse command line arguments and get an input stream
    struct opts *opts = NULL;
    struct cstream *cs = parse_args(argc, argv, &opts);
    if (!opts || (!cs && !opts->c))
    {
        free(opts);
        return 1;
    }

    // Create a vector to hold the current line
    struct vec *line = vec_init();

    struct parser *parser = create_parser();
    // Run the test loop
    rc = read_print_loop(cs, line, parser, opts);

    free(opts);
    free(line);
    if (cs)
    {
        cstream_free(cs);
        free(cs);
    }
    parser_free(parser);
    return rc;
}

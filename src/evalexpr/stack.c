#include "stack.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct token_stack token_op(char *op)
{
    struct token_stack t;
    if (strncmp(op, "**", 2) == 0)
        t.type = DOUBLE_STAR;
    else if (strncmp(op, "*", 1) == 0)
        t.type = MULT;
    if (strncmp(op, "+", 1) == 0)
        t.type = ADD;
    if (strncmp(op, "-", 1) == 0)
        t.type = MINUS;
    if (strncmp(op, "/", 1) == 0)
        t.type = DIV;
    if (strncmp(op, "%", 1) == 0)
        t.type = MOD;
    if (strncmp(op, "^", 1) == 0)
        t.type = POW;
    if (strncmp(op, "(", 1) == 0)
        t.type = POPEN;
    if (strncmp(op, ")", 1) == 0)
        t.type = PCLOSE;
    if (strncmp(op, "&&", 2) == 0)
        t.type = LOGICAL_AND;
    else if (strncmp(op, "||", 2) == 0)
        t.type = LOGICAL_OR;
    else if (strncmp(op, "&", 1) == 0)
        t.type = BINARY_AND;
    else if (strncmp(op, "|", 1) == 0)
        t.type = BINARY_OR;
    else if (strncmp(op, "!", 1) == 0)
        t.type = NOT;
    else if (strncmp(op, "~", 1) == 0)
        t.type = TILDE;
    return t;
}

// if the operation is valid, return the result, raise error otherwise
static int compute_errors(int a, int b, char op)
{
    if (op == '/')
    {
        if (b == 0)
            return INT_MIN;
        else
            return a / b;
    }
    else if (op == '%')
    {
        if (b == 0)
            return INT_MIN;
        else
            return a % b;
    }
    else if (b < 0)
        return INT_MIN;
    else
    {
        int res = 1;
        while (b > 0)
        {
            res *= a;
            b--;
        }
        return res;
    }
    return 0;
}

char *char_op(struct token_stack t)
{
    switch (t.type)
    {
    case DOUBLE_STAR:
        return "**";
    case POW:
        return "^";
    case MULT:
        return "*";
    case DIV:
        return "/";
    case MOD:
        return "%";
    case ADD:
        return "+";
    case MINUS:
        return "-";
    case UMINUS:
        return "@";
    case BINARY_AND:
        return "&";
    case BINARY_OR:
        return "|";
    case LOGICAL_AND:
        return "&&";
    case LOGICAL_OR:
        return "||";
    case NOT:
        return "!";
    case TILDE:
        return "~";
    default:
        return "+";
    }
}

// Compute nb1 and nb2 with op
int compute(struct token_stack nb1, struct token_stack nb2, char *op)
{
    if (strncmp(op, "+", 1) == 0)
        return nb1.data + nb2.data;
    else if (strncmp(op, "-", 1) == 0)
        return nb1.data - nb2.data;
    else if (strncmp(op, "**", 2) == 0)
        return compute_errors(nb1.data, nb2.data, '^');
    else if (strncmp(op, "*", 1) == 0)
        return nb1.data * nb2.data;
    else if (strncmp(op, "/", 1) == 0)
        return nb1.data / nb2.data;
    else if (strncmp(op, "%", 1) == 0)
        return nb1.data % nb2.data;
    else if (strncmp(op, "^", 1) == 0)
        return nb1.data ^ nb2.data;
    else if (strncmp(op, "&&", 2) == 0)
        return nb1.data && nb2.data;
    else if (strncmp(op, "||", 2) == 0)
        return nb1.data || nb2.data;
    else if (strncmp(op, "&", 1) == 0)
        return nb1.data & nb2.data;
    else if (strncmp(op, "|", 1) == 0)
        return nb1.data | nb2.data;
    else if (strncmp(op, "!", 1) == 0)
        return !nb1.data;
    else if (strncmp(op, "!", 1) == 0)
        return !nb1.data;
    else if (strncmp(op, "~", 1) == 0)
        return -nb1.data - 1;
    else
        return compute_errors(nb1.data, nb2.data, op[0]);
}

struct stack *stack_add(struct stack *s, struct token_stack t)
{
    struct stack *new = malloc(sizeof(struct stack));
    if (!new)
        return NULL;
    new->data = t;
    new->next = s;
    return new;
}

struct stack *stack_pop(struct stack *s)
{
    if (s == NULL)
        return NULL;
    struct stack *res = s->next;
    free(s);
    return res;
}

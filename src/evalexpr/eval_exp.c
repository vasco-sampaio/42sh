#include "eval_exp.h"

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"

// Return 1 if c is an operator
static int is_operator(char *op)
{
    if (strncmp(op, "+", 1) == 0)
        return 1;
    if (strncmp(op, "-", 1) == 0)
        return 1;
    if (strncmp(op, "/", 1) == 0)
        return 1;
    if (strncmp(op, "*", 1) == 0)
        return 1;
    if (strncmp(op, "^", 1) == 0)
        return 1;
    if (strncmp(op, "%", 1) == 0)
        return 1;
    if (strncmp(op, "&", 1) == 0)
        return 1;
    if (strncmp(op, "|", 1) == 0)
        return 1;
    if (strncmp(op, "!", 1) == 0)
        return 1;
    if (strncmp(op, "~", 1) == 0)
        return 1;
    if (strncmp(op, "(", 1) == 0)
        return 1;
    if (strncmp(op, ")", 1) == 0)
        return 1;
    return 0;
}

// Get the number from a string
static int mini_atoi(char *s, size_t *i)
{
    int res = 0;
    while (isdigit(s[*i]))
    {
        res *= 10;
        res += s[*i] - '0';
        *i = *i + 1;
    }
    return res;
}

// Evalutate expr according to the reverse polish notation
int eval_rpn(char *expr, int rpn)
{
    size_t i = 0;
    struct stack *nb_stack = NULL;
    while (expr[i] != '\0')
    {
        while (isspace(expr[i]))
            i++;
        if (expr[i] == '\0')
            break;
        if (isdigit(expr[i]))
        {
            struct token_stack t = { NB, mini_atoi(expr, &i) };
            nb_stack = stack_add(nb_stack, t);
        }
        else if (expr[i] == '@' && rpn == 0) // Unary minus
        {
            nb_stack->data.data = -nb_stack->data.data;
            i++;
        }
        else if (expr[i] == '!' && rpn == 0)
        {
            nb_stack->data.data = !nb_stack->data.data;
            i++;
        }
        else if (expr[i] == '~' && rpn == 0)
        {
            nb_stack->data.data = -nb_stack->data.data - 1;
            i++;
        }
        else if (is_operator(expr + i))
        {
            char *save = expr + i;
            int count = 0;
            while (expr[i] != '\0' && expr[i] != ' ')
            {
                i++;
                count++;
            }
            char *operator= strndup(save, count);
            if (nb_stack == NULL)
            {
                free(operator);
                return INT_MIN;
            }
            struct token_stack nb1 = nb_stack->data;
            nb_stack = stack_pop(nb_stack);
            if (nb_stack == NULL)
            {
                free(operator);
                return INT_MIN;
            }
            struct token_stack nb2 = nb_stack->data;
            nb_stack = stack_pop(nb_stack);
            struct token_stack res = { NB, compute(nb2, nb1, operator) };
            free(operator);
            if (res.data == INT_MIN)
                return res.data;
            nb_stack = stack_add(nb_stack, res);
        }
        else
            return INT_MIN;
    }
    struct token_stack res = nb_stack->data;
    nb_stack = stack_pop(nb_stack);
    if (nb_stack != NULL)
        return INT_MIN;
    return res.data; // Success
}

struct stack *op_pop(struct stack *st, char *res, size_t *j)
{
    struct token_stack poped = st->data;
    st = stack_pop(st);
    char *operator= char_op(poped);
    res[*j] = operator[0];

    if (strlen(operator) > 1)
    {
        *j = *j + 1;
        res[*j] = operator[1];
    }
    *j = *j + 1;
    res[*j] = ' ';
    *j = *j + 1;
    return st;
}

// Return true if the char at index i is a unary operator
static int is_unary(char *expr, size_t i, int state)
{
    if (state == 1)
        return 0;
    if (expr[i] != '+' && expr[i] != '-' && expr[i] != '!' && expr[i] != '~')
        return 0;
    if (i == 0) // begining of the expr
        return 1;
    if (expr[i - 1] == '(')
        return 1;
    if (is_operator(expr + i - 1))
        return 1;
    if (isspace(expr[i - 1]))
        return 1;
    return 0;
}

struct stack *append_op(struct stack *st, struct token_stack cur, char *res,
                        size_t *j)
{
    if (cur.type == POPEN)
        st = stack_add(st, cur);
    else
    {
        while (st != NULL && st->data.type != POPEN
               && (cur.type == PCLOSE || st->data.type / 10 >= cur.type / 10))
            st = op_pop(st, res, j);
        st = cur.type == PCLOSE ? stack_pop(st) : stack_add(st, cur);
    }
    return st;
}

char *to_polish(char *expr)
{
    size_t i = 0;
    struct stack *st = NULL;
    char *res = malloc(sizeof(char) * strlen(expr) * 2);
    size_t j = 0;
    int state = 0; // 1 if last was add nb, 2 if last was add op
    while (expr[i] != '\0')
    {
        while (isspace(expr[i]))
            i++;
        if (expr[i] == '\0')
            break;
        if (is_unary(expr, i, state))
        {
            if (expr[i] == '-')
            {
                struct token_stack uminus = { UMINUS, 0 };
                st = stack_add(st, uminus);
            }
            else if (expr[i] == '~')
            {
                struct token_stack tilde = { TILDE, 0 };
                st = stack_add(st, tilde);
            }
            else if (expr[i] == '!')
            {
                struct token_stack not = { NOT, 0 };
                st = stack_add(st, not );
            }
            i++;
        }
        else if (is_operator(expr + i))
        {
            struct token_stack cur = token_op(expr + i);
            if (cur.type == LOGICAL_OR || cur.type == LOGICAL_AND
                || cur.type == DOUBLE_STAR)
                i++;
            i++;
            st = append_op(st, cur, res, &j);
            state = 2;
        }
        else if (isdigit(expr[i]))
        {
            while (isdigit(expr[i]))
                res[j++] = expr[i++];
            res[j++] = ' ';
            state = 1;
        }
        else
            return NULL;
    }
    while (st != NULL) // Destack remaining operators
    {
        char *op = char_op(st->data);
        int k = 0;
        while (op[k] != '\0')
        {
            res[j++] = op[k];
            k++;
        }
        st = stack_pop(st);
    }
    res[j] = '\0';
    return res;
}

int eval_exp(char *expr)
{
    int valid = 0;
    for (size_t i = 0; expr[i] != '\0'; i++)
    {
        if (isdigit(expr[i]))
            valid = 1;
    }
    if (valid == 0)
        return INT_MIN;
    else
    {
        char *polish = to_polish(expr);
        if (polish == NULL)
            return INT_MIN;
        int res = eval_rpn(polish, 0);
        free(polish);
        return res;
    }
    return 0;
}

#ifndef STACK_H
#define STACK_H

#include "eval_exp.h"

/*
** Enum sorted in priority order
** Make a division per 10 to get the priority order
*/
enum type
{
    MINUS = 0,
    ADD = 1,
    MOD = 10,
    DIV = 11,
    MULT = 12,
    BINARY_AND = 13,
    BINARY_OR = 14,
    LOGICAL_AND = 15,
    LOGICAL_OR = 16,
    NOT = 17,
    POW = 20,
    DOUBLE_STAR = 21,
    UMINUS = 30,
    UPLUS = 31,
    TILDE = 32,
    PCLOSE = 40,
    POPEN = 41,
    NB
};

struct token_stack
{
    enum type type;
    int data;
};

struct stack
{
    struct token_stack data;
    struct stack *next;
};

// Return a token_stack according to its operator
struct token_stack token_op(char *op);

// Return the operator of a token_stack
char *char_op(struct token_stack);

// 0 if t1 = t2, 1 if t1>t2, -1 otherwise
int compute(struct token_stack t1, struct token_stack t2, char *op);

// Add an element in the stack and return the new head, NULL return new stack
struct stack *stack_add(struct stack *s, struct token_stack);

// Pop the head of the stack and return the new head
// Carreful ! Need to save the head before pop
struct stack *stack_pop(struct stack *s);

#endif

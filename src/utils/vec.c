#include <stdio.h>
#include <utils/alloc.h>
#include <utils/vec.h>

struct vec *vec_init(void)
{
    struct vec *vec = xmalloc(sizeof(struct vec));
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
    return vec;
}

void vec_destroy(struct vec *vec)
{
    free(vec->data);
    // this isn't strictly required, but makes debugging a lot easier:
    // the app will crash when using a destroyed vector, instead of writing into
    // freed memory (which may or may not crash the program, but will likely
    // result) in the program crashing at a later time
    vec->data = NULL;
}

static void vec_grow(struct vec *vec)
{
    size_t new_capacity;
    if (vec->capacity == 0)
        new_capacity = 10;
    else
        new_capacity = vec->capacity * 2;

    char *new_data = xrealloc(vec->data, new_capacity);
    vec->data = new_data;
    vec->capacity = new_capacity;
}

void vec_reset(struct vec *vec)
{
    vec->size = 0;
}

void vec_push(struct vec *vec, char c)
{
    if (vec->size == vec->capacity)
        vec_grow(vec);

    vec->data[vec->size++] = c;
}

char *vec_cstring(struct vec *vec)
{
    if (vec->size == 0 || vec->data[vec->size - 1] != '\0')
        vec_push(vec, '\0');
    return vec->data;
}

struct vec *vec_concat(struct vec *vec, struct vec *vec2)
{
    if (!vec)
    {
        vec = zalloc(sizeof(struct vec));
        vec->data = strdup("");
        vec->size = 1;
        vec->capacity = 1;
    }
    vec->size--;
    for (size_t i = 0; i < vec2->size; i++)
    {
        vec_push(vec, vec2->data[i]);
    }
    vec_cstring(vec);
    vec_reset(vec2);
    return vec;
}

void vec_print(struct vec *vec)
{
    for (size_t i = 0; i < vec->size; i++)
        printf("%c", vec->data[i]);
}

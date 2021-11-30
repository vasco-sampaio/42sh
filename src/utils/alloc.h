#pragma once

#include <stdlib.h>
#include <string.h>
#include <utils/attributes.h>

/** \brief Like malloc, but calls abort() in case of failure. */
static inline void *__malloc __alloc_size(1) xmalloc(size_t size)
{
    void *res = malloc(size);
    if (res == NULL && size != 0)
        abort();
    return res;
}

/** \brief Like realloc, but calls abort() in case of failure. */
static inline void *__alloc_size(2) xrealloc(void *ptr, size_t size)
{
    void *res = realloc(ptr, size);
    if (res == NULL && size != 0)
        abort();
    return res;
}

/** \brief Like malloc, but initializes memory to zero and calls abort() in case
 * of failure. */
static inline void *__malloc __alloc_size(1) zalloc(size_t size)
{
    void *res = xmalloc(size);
    memset(res, 0, size);
    return res;
}

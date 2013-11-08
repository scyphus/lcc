/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "vector.h"
#include "checkint.h"
#include <stdlib.h>
#include <string.h>

/*
 * Allocate struct vector
 */
struct vector *
vector_new(size_t sz)
{
    struct vector *vec;

    /* Check if allocation is needed */
    if ( NULL == (vec = malloc(sizeof(struct vector))) ) {
        return NULL;
    }
    bzero(vec, sizeof(struct vector));

    /* Allocate memory space for elements */
    if ( NULL == (vec->elems = malloc(sizeof(void *) * sz)) ) {
        free(vec);
        return NULL;
    }
    vec->size = 0;
    vec->max_size = sz;

    return vec;
}

/*
 * Free struct vector
 */
void
vector_delete(struct vector *vec)
{
    if ( NULL != vec ) {
        free(vec->elems);
        /* Free vector. */
        free(vec);
    }
}

/*
 * Clear
 */
void
vector_clear(struct vector *vec)
{
    vec->size = 0;
}

/*
 * Reserve size of vector
 */
struct vector *
vector_reserve(struct vector *vec, size_t sz)
{
    void **elems;
    /* Allocate memory space for elements */
    if ( NULL == (elems = realloc(vec->elems, sizeof(void *) * sz)) ) {
        /* Do not delete the old vector */
        return NULL;
    }
    vec->elems = elems;
    vec->max_size = sz;

    return vec;
}

/*
 * Get used size of vector
 */
size_t
vector_size(const struct vector *vec)
{
    return vec->size;
}

/*
 * Get used max size of vector
 */
size_t
vector_max_size(struct vector *vec)
{
    return vec->max_size;
}

/*
 * Push a element to the vector.
 */
struct vector *
vector_push_back(struct vector *vec, void *elem)
{
    size_t new_sz;
    int err;

    /* Calculate new size of the vector */
    new_sz = check_size_t_add(vec->size, 1, &err);
    if ( err ) {
        /* Out of range */
        return NULL;
    }

    /* Check the max size */
    if ( new_sz > vec->max_size ) {
        /* Overflow */
        return NULL;
    }

    vec->elems[new_sz - 1] = elem;
    vec->size = new_sz;

    return vec;
}

/*
 * Pop a element from the vector.
 */
void *
vector_pop_back(struct vector *vec)
{
    void *elem;

    if ( 0 == vec->size ) {
        return NULL;
    }

    elem = vec->elems[vec->size - 1];
    vec->size = vec->size - 1;

    return elem;
}

/*
 * Get a element
 */
void *
vector_at(const struct vector *vec, size_t pos)
{
    void *elem;

    if ( vec->size <= pos ) {
        return NULL;
    }
    elem = vec->elems[pos];

    return elem;
}

/*
 * Replace a element
 */
void *
vector_replace(struct vector *vec, size_t pos, void *new)
{
    void *elem;

    if ( vec->size <= pos ) {
        return NULL;
    }
    elem = vec->elems[pos];
    vec->elems[pos] = new;

    return elem;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

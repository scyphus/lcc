/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "mvector.h"
#include "checkint.h"
#include <stdlib.h>
#include <string.h>

/*
 * Allocate a mutable vector instance and initialize it
 */
struct mvector *
mvector_new(void)
{
    struct mvector *vec;

    /* Allocate the memory for struct mvector */
    vec = malloc(sizeof(struct mvector));
    if ( NULL == vec ) {
        return NULL;
    }
    (void)memset(vec, 0, sizeof(struct mvector));

    /* Allocate memory space for elements */
    vec->elems = malloc(sizeof(void *) * MVECTOR_DEFAULT_MAXSIZE);
    if ( NULL == vec->elems ) {
        free(vec);
        return NULL;
    }
    vec->size = 0;
    vec->max_size = MVECTOR_DEFAULT_MAXSIZE;

    return vec;
}

/*
 * Free the mutable vector instance
 */
void
mvector_delete(struct mvector *vec)
{
    if ( NULL != vec ) {
        free(vec->elems);
        free(vec);
    }
}

/*
 * Clear the mutable vector
 */
void
mvector_clear(struct mvector *vec)
{
    vec->size = 0;
}

/*
 * Get used size of the mutable vector
 */
size_t
mvector_size(const struct mvector *vec)
{
    return vec->size;
}

/*
 * Push an element to the mutable vector
 */
struct mvector *
mvector_push_back(struct mvector *vec, void *elem)
{
    size_t new_sz;
    size_t max_sz;
    int err;
    void **elems;

    /* Calculate new size of the mutable vector */
    new_sz = check_size_t_add(vec->size, 1, &err);
    if ( err ) {
        /* Out of range */
        return NULL;
    }

    /* Check the max size */
    if ( new_sz > vec->max_size ) {
        /* The allocated space is already full */
        max_sz = check_size_t_add(vec->max_size, MVECTOR_DEFAULT_MAXSIZE, &err);
        if ( err ) {
            /* Failed to extend the space */
            return NULL;
        }

        /* Allocate memory space for elements */
        elems = realloc(vec->elems, sizeof(void *) * max_sz);
        if ( NULL == elems ) {
            /* Do not delete the old vector */
            return NULL;
        }
        vec->elems = elems;
        vec->max_size = max_sz;
    }

    vec->elems[new_sz - 1] = elem;
    vec->size = new_sz;

    return vec;
}

/*
 * Pop an element from the mutable vector
 */
void *
mvector_pop_back(struct mvector *vec)
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
 * Get an element from the mutable vector
 */
void *
mvector_at(const struct mvector *vec, size_t pos)
{
    void *elem;

    if ( vec->size <= pos ) {
        return NULL;
    }
    elem = vec->elems[pos];

    return elem;
}

/*
 * Replace an element at the specified position
 */
void *
mvector_replace(struct mvector *vec, size_t pos, void *new)
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

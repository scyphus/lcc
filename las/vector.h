/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _VECTOR_H
#define _VECTOR_H

#include <stddef.h>

/*
 * struct vector
 */
struct vector {
    /* Elements */
    void **elems;
    /* Size of elements */
    size_t size;
    /* Allocated size */
    size_t max_size;
};

#ifdef __cplusplus
extern "C" {
#endif

    struct vector * vector_new(size_t);
    void vector_delete(struct vector *);
    void vector_clear(struct vector *);
    struct vector * vector_reserve(struct vector *, size_t);
    size_t vector_size(const struct vector *);
    size_t vector_max_size(struct vector *);
    struct vector * vector_push_back(struct vector *, void *);
    void * vector_pop_back(struct vector *);
    void * vector_at(const struct vector *, size_t);
    void * vector_replace(struct vector *, size_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* _VECTOR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

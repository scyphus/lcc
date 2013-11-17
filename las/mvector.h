/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _MVECTOR_H
#define _MVECTOR_H

#include <stddef.h>

#define MVECTOR_DEFAULT_MAXSIZE 8
#define MVECTOR_ALIGNMENT_SIZE  8

/*
 * Mutable vector
 */
struct mvector {
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

    struct mvector * mvector_new(void);
    void mvector_delete(struct mvector *);
    void mvector_clear(struct mvector *);
    size_t mvector_size(const struct mvector *);
    struct mvector * mvector_push_back(struct mvector *, void *);
    void * mvector_pop_back(struct mvector *);
    void * mvector_at(const struct mvector *, size_t);
    void * mvector_replace(struct mvector *, size_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* _MVECTOR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

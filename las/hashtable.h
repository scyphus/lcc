/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <sys/types.h>  /* for ssize_t etc.. */

struct hashtable {
    /* Hash space */
    ssize_t hashsize;
    /* Hash function */
    ssize_t (*hashfunc)(const void *);
    /* Function for comparing key */
    int (*compare)(const void *, const void *);
    /* Use list temporally */
    void **hashdata;
};

#ifdef __cplusplus
extern "C" {
#endif

    struct hashtable *
    hashtable_new(ssize_t, ssize_t (*)(const void *),
                  int (*)(const void *, const void *));
    int hashtable_set(struct hashtable *, void *, void *, void **);
    int hashtable_get(struct hashtable *, void *, void **);
    int hashtable_unset(struct hashtable *, void *, void **);


#ifdef __cplusplus
}
#endif

#endif /* _HASHTABLE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

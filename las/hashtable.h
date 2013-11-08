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
    /** hash space */
    ssize_t hashsize;
    /** hash function */
    ssize_t (*hashfunc)(const void *);
    /** function for comparing key */
    int (*compare)(const void *, const void *);
    /** use list temporally */
    void **hashdata;
};

#ifdef __cplusplus
extern "C" {
#endif

    struct hashtable * hashtable_init(ssize_t, ssize_t (*)(const void *),
                                      int (*)(const void *, const void *));
    int hashtable_set(void *, void *, struct hashtable *);
    int hashtable_get(void **, void *, struct hashtable *);
    int hashtable_unset(void *, struct hashtable *);


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

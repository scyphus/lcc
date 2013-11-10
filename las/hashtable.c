/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "hashtable.h"
#include <stdlib.h>
#include <string.h>     /* for memset */
#include <sys/types.h>  /* for ssize_t etc.. */

struct _list {
    void *key;
    void *value;
    struct _list *next;
};

/*
 * Create a new hash table instance
 */
struct hashtable *
hashtable_new(ssize_t hashsize, ssize_t (*hashfunc)(const void *),
              int (*compare)(const void *, const void *))
{
    struct hashtable *ht;

    ht = malloc(sizeof(struct hashtable *) * hashsize);
    if ( NULL == ht ) {
        return NULL;
    }

    if ( hashsize <= 0 ) {
        free(ht);
        return NULL;
    }
    ht->hashsize = hashsize;
    ht->hashfunc = hashfunc;
    ht->compare = compare;
    ht->hashdata = malloc(sizeof(struct _list *) * hashsize);
    if ( NULL == ht->hashdata ) {
        free(ht);
        return NULL;
    }
    (void)memset(ht->hashdata, 0, sizeof(struct _list *) * hashsize);

    return ht;
}

/*
 * Set a value for a key
 */
int
hashtable_set(struct hashtable *ht, void *key, void *value, void **oldvalue)
{
    ssize_t idx;
    struct _list *list, *prev;

    idx = ht->hashfunc(key);
    list = ht->hashdata[idx];
    if ( NULL == list ) {
        list = malloc(sizeof(struct _list));
        if ( NULL == list ) {
            return -1;
        }
        list->key = key;
        list->value = value;
        list->next = NULL;
        ht->hashdata[idx] = (void *) list;
        *oldvalue = NULL;

        return 0;
    } else {
        prev = NULL;
        while ( NULL != list ) {
            if ( 0 == ht->compare(list->key, key) ) {
                /* Overwrite */
                *oldvalue = list->value;
                list->value = value;
                return 1;
            }
            prev = list;
            list = list->next;
        }
        prev->next = malloc(sizeof(struct _list));
        if ( NULL == prev->next ) {
            return -1;
        }
        prev->next->key = key;
        prev->next->value = value;
        prev->next->next = NULL;
        *oldvalue = NULL;

        return 0;
    }
}

/*
 * Get the value for a key
 */
int
hashtable_get(struct hashtable *ht, void *key, void **value)
{
    ssize_t idx;
    struct _list *list;

    idx = ht->hashfunc(key);
    list = (struct _list *) ht->hashdata[idx];
    while ( NULL != list ) {
        if ( 0 == ht->compare(list->key, key) ) {
            /* Found */
            *value = list->value;
            return 1;
        }
        list = list->next;
    }

    /* Not found */
    return 0;
}

/*
 * Unset the stored value for a key
 */
int
hashtable_unset(struct hashtable *ht, void *key, void **value)
{
    ssize_t idx;
    struct _list *list, *prev;

    idx = ht->hashfunc(key);
    list = (struct _list *) ht->hashdata[idx];
    prev = NULL;
    while ( NULL != list ) {
        if ( 0 == ht->compare(list->key, key) ) {
            /* Found the value */
            *value = list->value;
            if ( NULL != prev ) {
                prev->next = list->next;
                free(list);
            } else {
                free(list);
                ht->hashdata[idx] = NULL;
            }

            /* Succeeded */
            return 1;
        }
        prev = list;
        list = list->next;
    }

    /* Failed */
    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

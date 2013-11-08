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

struct hashtable *
hashtable_init(ssize_t hashsize, ssize_t (*hashfunc)(const void *),
               int (*compare)(const void *, const void *))
{
    struct hashtable *ht;

    if ( NULL == (ht = malloc(sizeof(struct hashtable *) * hashsize)) ) {
         exit(1);
    }

    if ( hashsize <= 0 ) {
        exit(1);
    }
    ht->hashsize = hashsize;
    ht->hashfunc = hashfunc;
    ht->compare = compare;
    if ( NULL == (ht->hashdata = malloc(sizeof(struct _list *) * hashsize)) ) {
         exit(1);
    }
    memset(ht->hashdata, 0, sizeof(struct _list *) * hashsize);

    return ht;
}

int
hashtable_set(void *key, void *value, struct hashtable *ht)
{
    ssize_t idx;
    struct _list *list, *prev;

    idx = ht->hashfunc(key);
    list = ht->hashdata[idx];
    if ( NULL == list ) {
        if ( NULL == (list = malloc(sizeof(struct _list))) ) {
            exit(1);
        }
        list->key = key;
        list->value = value;
        list->next = NULL;
        ht->hashdata[idx] = (void *) list;
        return 0;
    } else {
        prev = NULL;
        while ( NULL != list ) {
            if ( 0 == ht->compare(list->key, key) ) {
                /* overwrite */
                list->value = value;
                return 0;
            }
            prev = list;
            list = list->next;
        }
        if ( NULL == (prev->next = malloc(sizeof(struct _list))) ) {
            exit(1);
        }
        prev->next->key = key;
        prev->next->value = value;
        prev->next->next = NULL;
        return 0;
    }
}

int
hashtable_get(void **value, void *key, struct hashtable *ht)
{
    ssize_t idx;
    struct _list *list;

    idx = ht->hashfunc(key);
    list = (struct _list *) ht->hashdata[idx];
    while ( NULL != list ) {
        if ( 0 == ht->compare(list->key, key) ) {
            *value = list->value;
            return 0;
        }
        list = list->next;
    }
    return 1;
}

int
hashtable_unset(void *key, struct hashtable *ht)
{
    ssize_t idx;
    struct _list *list, *prev;

    idx = ht->hashfunc(key);
    list = (struct _list *) ht->hashdata[idx];
    prev = NULL;
    while ( NULL != list ) {
        if ( 0 == ht->compare(list->key, key) ) {
            if ( NULL != prev ) {
                prev->next = list->next;
                free(list);
            } else {
                free(list);
                ht->hashdata[idx] = NULL;
            }
            return 0;
        }
        prev = list;
        list = list->next;
    }
    return 1;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _LAS_H
#define _LAS_H

#include "token.h"
#include <stdio.h>
#include <unistd.h>

/*
 * String (can be binary)
 */
typedef struct string {
    unsigned char *s;
    size_t len;
} string_t;

/*
 * Source code
 */
typedef struct scode {
    char *fname;
    string_t str;
} scode_t;

/*
 * Token
 */
typedef struct token {
    token_type_t type;
    union {
        /* Symbol */
        char *sym;
        /* Numerical value */
        char *num;
    } val;
    /* Debugging information */
    scode_t *scode;
    struct {
        size_t l;
        size_t c;
    } pos;
    off_t off;
} token_t;

/*
 * An entry of token
 */
typedef struct token_queue_entry token_queue_entry_t;
struct token_queue_entry {
    token_t *token;
    token_queue_entry_t *next;
};

/*
 * A queue of tokens
 */
typedef struct token_queue {
    token_queue_entry_t *head;
    token_queue_entry_t *tail;
    token_queue_entry_t *cur;
} token_queue_t;

/*
 * Tokenized code (output of the first process)
 */
typedef struct tcode {
    token_queue_t *token_queue;
    /* Debugging information */
    scode_t *scode;
} tcode_t;

/*
 * Code tree
 */
typedef struct code_tree_node code_tree_node_t;
struct code_tree_node {
    tcode_t *tcode;
    size_t nr;
    code_tree_node_t *children;
};

/*
 * Preprocessed code (output of the second process)
 */
typedef struct pcode {
    token_queue_t *token_queue;
    /* Code tree */
    code_tree_node_t *tree;
} pcode_t;

/*
 * Tokenizer
 */
typedef struct tokenizer {
    scode_t *scode;
    off_t cur;
    /* Debugging information */
    struct {
        size_t l;
        size_t c;
    } pos;
} tokenizer_t;


/*
 * Preprocessor
 */
typedef struct preprocessor {
    tcode_t *tcode;
    /*hashtable*/
} preprocessor_t;



#ifdef __cplusplus
extern "C" {
#endif

    tcode_t * tokenize(scode_t *);
    pcode_t * preprocess(const char *);

    scode_t * scode_read(const char *);
    void scode_delete(scode_t *);


#ifdef __cplusplus
}
#endif

#endif /* _LAS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

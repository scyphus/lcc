/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _LAS_TOKEN_H
#define _LAS_TOKEN_H

#include <stddef.h>
#include <sys/types.h>          /* off_t */

typedef enum _token_type {
    /* Undefined token */
    TOK_UNDEF = -2,

    /* Invaid token */
    TOK_INVAL = -1,

    /* End-of-line */
    TOK_EOL = 0,

    /* # */
    TOK_SHARP,

    /* Symbol */
    TOK_SYMBOL,

    /* Integer */
    TOK_BININT,
    TOK_OCTINT,
    TOK_DECINT,
    TOK_HEXINT,

    /* Comma */
    TOK_COMMA,

    /* COLON */
    TOK_COLON,

    /* SEMICOLON */
    TOK_SEMICOLON,

    TOK_LBRACKET,
    TOK_RBRACKET,

    /* Operators */
    TOK_OP_PLUS,
    TOK_OP_MINUS,
    TOK_OP_MUL,
    TOK_OP_DIV,

} token_type_t;


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LAS_TOKEN_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

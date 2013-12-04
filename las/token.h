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

    /* Keywords */
    TOK_KW_GLOBAL,
    TOK_KW_BYTE,
    TOK_KW_WORD,
    TOK_KW_DWORD,
    TOK_KW_QWORD,

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
    TOK_LPAREN,
    TOK_RPAREN,

    /* Operators */
    TOK_OP_PLUS,
    TOK_OP_MINUS,
    TOK_OP_MUL,
    TOK_OP_DIV,
    TOK_OP_TILDE,
    TOK_OP_LSHIFT,
    TOK_OP_RSHIFT,
    TOK_OP_BAR,
    TOK_OP_AMP,
    TOK_OP_XOR,

} token_type_t;

#endif /* _LAS_TOKEN_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

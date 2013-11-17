/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _LAS_EXPR_H
#define _LAS_EXPR_H

#include "vector.h"
#include "token.h"
#include <stdint.h>

typedef struct vector expr_vec_t;

/*
 * Types of operator
 */
typedef enum _expr_operator_type {
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_LSHIFT,
    OP_RSHIFT,
    OP_TILDE,
    OP_BAR,
    OP_AMP,
    OP_XOR,
} expr_operator_type_t;

/*
 * Types of *fix
 */
typedef enum _expr_fix_type {
    FIX_PREFIX,
    FIX_INFIX,
} expr_fix_type_t;

/*
 * Types of expression
 */
typedef enum _expr_type {
    EXPR_VAR,
    EXPR_INT,
    EXPR_OP,
} expr_type_t;

/*
 * Operation
 */
typedef struct _expr_op_t {
    expr_operator_type_t type;
    expr_fix_type_t fix_type;   /* Prefix or infix */
    expr_vec_t *args;           /* Operands */
} expr_op_t;

/*
 * Expression
 */
typedef struct expr {
    expr_type_t type;
    union {
        char *var;
        uint64_t i;
        expr_op_t op;
    } u;
} expr_t;

#ifdef __cplusplus
extern "C" {
#endif

    expr_t * expr_infix_operator(expr_operator_type_t, expr_t *, expr_t *);
    expr_t * expr_prefix_operator(expr_operator_type_t, expr_t *);
    expr_t * expr_var(char *);
    expr_t * expr_int(uint64_t);
    void expr_free(expr_t *);

#ifdef __cplusplus
}
#endif

#endif /* _LAS_EXPR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

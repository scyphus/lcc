/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "vector.h"
#include "expr.h"
#include "parser.h"
#include "las.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * Prototype declarations
 */



/*
 * Create new infix operator
 */
expr_t *
expr_infix_operator(expr_operator_type_t type, expr_t *expr0, expr_t *expr1)
{
    expr_t *expr;
    expr_vec_t *args;

    /* The number of arguments is 2 for infix operator */
    args = vector_new(2);
    if ( NULL == vector_push_back(args, expr0) ) {
        vector_delete(args);
        return NULL;
    }
    if ( NULL == vector_push_back(args, expr1) ) {
        vector_delete(args);
        return NULL;
    }

    /* Allocate new expression for an infix operator */
    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        vector_delete(args);
        return NULL;
    }
    expr->type = EXPR_OP;
    expr->u.op.type = type;
    expr->u.op.fix_type = FIX_INFIX;
    expr->u.op.args = args;

    return expr;
}

/*
 * Create new prefix operator
 */
expr_t *
expr_prefix_operator(expr_operator_type_t type, expr_t *expr0)
{
    expr_t *expr;
    expr_vec_t *args;

    /* The number of arguments is 1 for prefix operator */
    args = vector_new(1);
    if ( NULL == vector_push_back(args, expr0) ) {
        vector_delete(args);
        return NULL;
    }

    /* Allocate new expression for a prefix operator */
    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        vector_delete(args);
        return NULL;
    }
    expr->type = EXPR_OP;
    expr->u.op.type = type;
    expr->u.op.fix_type = FIX_PREFIX;
    expr->u.op.args = args;

    return expr;
}

/*
 * Variable (symbol or label) expression
 */
expr_t *
expr_var(char *var)
{
    expr_t *expr;

    /* Allocate new expression for a variable */
    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        return NULL;
    }
    expr->type = EXPR_VAR;
    expr->u.var = strdup(var);
    if ( NULL == expr->u.var ) {
        free(expr);
        return NULL;
    }

    return expr;
}

/*
 * Integer expression
 */
expr_t *
expr_int(uint64_t val)
{
    expr_t *expr;

    /* Allocate new expression for an integer value */
    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        return NULL;
    }
    expr->type = EXPR_INT;
    expr->u.i = val;

    return expr;
}

/*
 * Free expression instance
 */
void
expr_free(expr_t *expr)
{
    size_t i;

    switch ( expr->type ) {
    case EXPR_VAR:
        free(expr->u.var);
        break;
    case EXPR_INT:
        /* Do nothing else */
        break;
    case EXPR_OP:
        switch ( expr->u.op.fix_type ) {
        case FIX_PREFIX:
        case FIX_INFIX:
            for ( i = 0; i < vector_size(expr->u.op.args); i++ ) {
                expr_free(vector_at(expr->u.op.args, i));
            }
            vector_delete(expr->u.op.args);
            break;
        default:
            /* Must not reach here */
            ;
        }
        break;
    default:
        ;
    }
    free(expr);
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

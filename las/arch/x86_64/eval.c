/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "eval.h"
#include "reg.h"
#include <stdlib.h>
#include <stdint.h>

/*
 * Prototype declarations
 */
static x86_64_val_t * _eval_expr_var(expr_t *);
static x86_64_val_t * _eval_expr_int(expr_t *);
static x86_64_val_t * _eval_expr_op(expr_t *);
static x86_64_val_t * _eval_expr(expr_t *);

/*
 * Evaluate var expression
 */
static x86_64_val_t *
_eval_expr_var(expr_t *expr)
{
    x86_64_reg_t reg;

    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol */
    } else {
        /* Register */
    }

    return NULL;
}

/*
 * Evaluate integer expression
 */
static x86_64_val_t *
_eval_expr_int(expr_t *expr)
{
    x86_64_val_t *val;

    val = malloc(sizeof(x86_64_val_t));
    if ( NULL == val ) {
        return NULL;
    }
    val->type = VAL_IMM;
    val->u.imm = expr->u.i;

    return val;
}

/*
 * Evaluate operand
 */
static x86_64_val_t *
_eval_expr_op(expr_t *expr)
{
    x86_64_val_t *val;
    x86_64_val_t *lval;
    x86_64_val_t *rval;
    expr_t *expr0;
    expr_t *expr1;

    val = malloc(sizeof(x86_64_val_t));
    if ( NULL == val ) {
        return NULL;
    }

    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        lval = _eval_expr(expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            /* Do nothing */
            break;
        case OP_MINUS:
            /* Minus */
            val->u.imm = -lval->u.imm;
            break;
        case OP_TILDE:
            /* Bitwise not */
            val->u.imm = ~lval->u.imm;
            break;
        default:
            ;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        lval = _eval_expr(expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        rval = _eval_expr(expr1);
        if ( NULL == rval ) {
            /* Free lval */
            free(val);
            free(lval);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            val->type = VAL_IMM;
            val->u.imm = lval->u.imm + rval->u.imm;
            break;
        case OP_MINUS:
            val->type = VAL_IMM;
            val->u.imm = lval->u.imm - rval->u.imm;
            break;
        case OP_MUL:
            val->type = VAL_IMM;
            val->u.imm = lval->u.imm * rval->u.imm;
            break;
        case OP_DIV:
            val->type = VAL_IMM;
            val->u.imm = lval->u.imm / rval->u.imm;
            break;
        default:
            ;
        }

    }

    return val;
}


/*
 * Evaluate the expression
 */
static x86_64_val_t *
_eval_expr(expr_t *expr)
{
    x86_64_val_t *val;

    switch ( expr->type ) {
    case EXPR_VAR:
        val = _eval_expr_var(expr);
        break;
    case EXPR_INT:
        val = _eval_expr_int(expr);
        break;
    case EXPR_OP:
        val = _eval_expr_op(expr);
        break;
    }

    return val;
}

/*
 * Evaluate the expression
 */
x86_64_val_t *
x86_64_eval_expr(expr_t *expr)
{
    return _eval_expr(expr);
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

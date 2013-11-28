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
    x86_64_val_t *val;
    x86_64_reg_t reg;

    val = malloc(sizeof(x86_64_val_t));
    if ( NULL == val ) {
        return NULL;
    }

    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol: To be implemented */
        val->type = X86_64_VAL_IMM;
        val->u.imm = 0;
        val->opsize = 0;
    } else {
        /* Register */
        val->type = X86_64_VAL_REG;
        val->u.reg = reg;
        val->opsize = regsize(reg);
    }

    return val;
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
    val->type = X86_64_VAL_IMM;
    val->u.imm = expr->u.i;
    val->opsize = 0;

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
    val->opsize = 0;

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
            val->type = X86_64_VAL_IMM;
            val->u.imm = lval->u.imm + rval->u.imm;
            break;
        case OP_MINUS:
            val->type = X86_64_VAL_IMM;
            val->u.imm = lval->u.imm - rval->u.imm;
            break;
        case OP_MUL:
            val->type = X86_64_VAL_IMM;
            val->u.imm = lval->u.imm * rval->u.imm;
            break;
        case OP_DIV:
            val->type = X86_64_VAL_IMM;
            val->u.imm = lval->u.imm / rval->u.imm;
            break;
        default:
            ;
        }

    }

    return val;
}

/*
 * Evaluate the expression (static function)
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
 * Evaluate the operand (immediate value or register)
 */
static x86_64_val_t *
_eval_expr_imm_or_reg(expr_t *expr)
{
    x86_64_val_t *val;

    switch ( expr->type ) {
    case EXPR_VAR:
        /* Register or immediate value (symbol) */
        val = _eval_expr_var(expr);
        break;
    case EXPR_INT:
        /* Immediate value */
        val = _eval_expr_int(expr);
        break;
    case EXPR_OP:
        /* Immediate value */
        val = _eval_expr_op(expr);
        break;
    default:
        val = NULL;
    }

    /* Verify the returned value */
    if ( NULL == val ) {
        return NULL;
    } else if ( X86_64_VAL_REG != val->type && X86_64_VAL_IMM != val->type ) {
        free(val);
        return NULL;
    }

    return val;
}

/*
 * Evaluate the operand (address operand)
 */
static x86_64_val_t *
_eval_expr_addr(expr_t *expr)
{
    /* To be implemented */
    return NULL;
}



/*
 * Evaluate an operand
 */
x86_64_val_t *
x86_64_eval_operand(operand_t *op)
{
    x86_64_val_t *val;

    if ( OPERAND_EXPR == op->type ) {
        /* Immediate value or register */
        val = _eval_expr_imm_or_reg(op->op.expr);
    } else {
        /* Address */
        val = _eval_expr_addr(op->op.expr);
    }

    return val;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

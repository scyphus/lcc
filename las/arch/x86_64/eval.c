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
        val->sopsize = 0;
    } else {
        /* Register */
        val->type = X86_64_VAL_REG;
        val->u.reg = reg;
        val->sopsize = 0;
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
    val->sopsize = 0;

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
    val->sopsize = 0;

    /* Refactoring is required... */
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
            if ( X86_64_VAL_IMM == lval->type
                 && X86_64_VAL_IMM == rval->type ) {
                val->type = X86_64_VAL_IMM;
                val->u.imm = lval->u.imm + rval->u.imm;
                val->sopsize = 0;
            } else if ( X86_64_VAL_REG == lval->type
                        && X86_64_VAL_IMM == rval->type ) {
                /* Base register + Displacement */
                val->type = X86_64_VAL_ADDR;
                val->u.addr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                val->u.addr.base = lval->u.reg;
                val->u.addr.disp = rval->u.imm;
                val->u.addr.saddrsize = 0;
                val->sopsize = 0;
            } else if ( X86_64_VAL_IMM == lval->type
                        && X86_64_VAL_REG == rval->type ) {
                /* Base register + Displacement */
                val->type = X86_64_VAL_ADDR;
                val->u.addr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                val->u.addr.base = rval->u.reg;
                val->u.addr.disp = lval->u.imm;
                val->u.addr.saddrsize = 0;
                val->sopsize = 0;
            } else if ( X86_64_VAL_ADDR == lval->type ) {
                if ( X86_64_VAL_REG == rval->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & lval->u.addr.flags ) {
                        /* Invalid syntax */
                        free(val);
                        free(lval);
                        free(rval);
                        return NULL;
                    }
                    val->type = X86_64_VAL_ADDR;
                    val->u.addr.flags = X86_64_ADDR_BASE | lval->u.addr.flags;
                    val->u.addr.base = rval->u.reg;
                    val->u.addr.saddrsize = 0;
                    val->sopsize = 0;
                } else if ( X86_64_VAL_IMM == rval->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & lval->u.addr.flags ) {
                        /* Invalid syntax */
                        free(val);
                        free(lval);
                        free(rval);
                        return NULL;
                    }
                    val->type = X86_64_VAL_ADDR;
                    val->u.addr.flags = X86_64_ADDR_DISP | lval->u.addr.flags;
                    val->u.addr.disp = rval->u.imm;
                    val->u.addr.saddrsize = 0;
                    val->sopsize = 0;
                }
            } else if ( X86_64_VAL_ADDR == rval->type ) {
                if ( X86_64_VAL_REG == lval->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & rval->u.addr.flags ) {
                        /* Invalid syntax */
                        free(val);
                        free(lval);
                        free(rval);
                        return NULL;
                    }
                    val->type = X86_64_VAL_ADDR;
                    val->u.addr.flags = X86_64_ADDR_BASE | rval->u.addr.flags;
                    val->u.addr.base = lval->u.reg;
                    val->u.addr.saddrsize = 0;
                    val->sopsize = 0;
                } else if ( X86_64_VAL_IMM == lval->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & rval->u.addr.flags ) {
                        /* Invalid syntax */
                        free(val);
                        free(lval);
                        free(rval);
                        return NULL;
                    }
                    val->type = X86_64_VAL_ADDR;
                    val->u.addr.flags = X86_64_ADDR_DISP | rval->u.addr.flags;
                    val->u.addr.disp = lval->u.imm;
                    val->u.addr.saddrsize = 0;
                    val->sopsize = 0;
                }
            } else {
                /* Invalid */
                free(val);
                free(lval);
                free(rval);
                return NULL;
            }
            break;
        case OP_MINUS:
            if ( X86_64_VAL_IMM == lval->type
                 && X86_64_VAL_IMM == rval->type ) {
                val->type = X86_64_VAL_IMM;
                val->u.imm = lval->u.imm - rval->u.imm;
                val->sopsize = 0;
            } else {
                /* Invalid */
                free(val);
                free(lval);
                free(rval);
                return NULL;
            }
            break;
        case OP_MUL:
            if ( X86_64_VAL_IMM == lval->type
                 && X86_64_VAL_IMM == rval->type ) {
                val->type = X86_64_VAL_IMM;
                val->u.imm = lval->u.imm * rval->u.imm;
                val->sopsize = 0;
            } else if ( X86_64_VAL_IMM == lval->type
                        && X86_64_VAL_REG == rval->type ) {
                val->type = X86_64_VAL_ADDR;
                val->u.addr.flags = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                val->u.addr.offset = rval->u.reg;
                val->u.addr.scale = lval->u.imm;
                val->u.addr.saddrsize = 0;
                val->sopsize = 0;
            } else if ( X86_64_VAL_REG == lval->type
                        && X86_64_VAL_IMM == rval->type ) {
                val->type = X86_64_VAL_ADDR;
                val->u.addr.flags = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                val->u.addr.offset = lval->u.reg;
                val->u.addr.scale = rval->u.imm;
                val->u.addr.saddrsize = 0;
                val->sopsize = 0;
            } else {
                /* Invalid */
                free(val);
                free(lval);
                free(rval);
                return NULL;
            }
            break;
        case OP_DIV:
            if ( X86_64_VAL_IMM == lval->type
                 && X86_64_VAL_IMM == rval->type ) {
                val->type = X86_64_VAL_IMM;
                val->u.imm = lval->u.imm / rval->u.imm;
                val->sopsize = 0;
            } else {
                /* Invalid */
                free(val);
                free(lval);
                free(rval);
                return NULL;
            }
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
    default:
        val = NULL;
    }

    return val;
}

/*
 * Evaluate the operand (immediate value or register)
 */
static x86_64_val_t *
_eval_expr_imm_or_reg(expr_t *expr)
{
    x86_64_val_t *val;

    val = _eval_expr(expr);

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
_eval_expr_addr(pexpr_t *pexpr)
{
    x86_64_val_t *val;
    x86_64_reg_t reg;
    int64_t imm;

    val = _eval_expr(pexpr->expr);
    if ( NULL == val ) {
        return NULL;
    }

    /* Convert the type to address operand */
    if ( X86_64_VAL_REG == val->type ) {
        reg = val->u.reg;
        val->type = X86_64_VAL_ADDR;
        val->u.addr.flags = X86_64_ADDR_BASE;
        val->u.addr.base = reg;
        val->u.addr.saddrsize = val->sopsize;
        val->sopsize = 0;
    } else if ( X86_64_VAL_IMM == val->type ) {
        imm = val->u.imm;
        val->type = X86_64_VAL_ADDR;
        val->u.addr.flags = X86_64_ADDR_DISP;
        val->u.addr.disp = imm;
        val->u.addr.saddrsize = val->sopsize;
        val->sopsize = 0;
    }

    /* Check the operand prefix */
    switch ( pexpr->prefix ) {
    case SIZE_PREFIX_BYTE:
        val->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        val->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        val->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        val->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Verify the returned value */
    if ( X86_64_VAL_ADDR != val->type ) {
        free(val);
        return NULL;
    }

    return val;
}

/*
 * Evaluate an operand
 */
x86_64_val_t *
x86_64_eval_operand(const x86_64_label_table_t *ltbl, operand_t *op)
{
    x86_64_val_t *val;
    size_t sz;

    if ( OPERAND_EXPR == op->type ) {
        /* Immediate value or register */
        val = _eval_expr_imm_or_reg(op->op.expr);
    } else {
        /* Address */
        val = _eval_expr_addr(op->op.pexpr);
    }
    /* Check the returned value */
    if ( NULL == val ) {
        return NULL;
    }

    /* Check the operand prefix */
    switch ( op->prefix ) {
    case SIZE_PREFIX_BYTE:
        val->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        val->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        val->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        val->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Complement and validate the operand size and address size */
    switch ( val->type ) {
    case X86_64_VAL_IMM:
        /* Check nothing */
        break;
    case X86_64_VAL_REG:
        sz = regsize(val->u.reg);
        if ( 0 != val->sopsize && sz != val->sopsize ) {
            /* Invalid operand size */
            free(val);
            return NULL;
        }
        val->sopsize = sz;
        break;
    case X86_64_VAL_ADDR:
        if ( X86_64_ADDR_BASE & val->u.addr.flags ) {
            sz = regsize(val->u.addr.base);
            if ( 0 != val->u.addr.saddrsize && sz != val->u.addr.saddrsize ) {
                /* Invalid operand size */
                free(val);
                return NULL;
            }
            val->u.addr.saddrsize = sz;
        }
        if ( X86_64_ADDR_OFFSET & val->u.addr.flags ) {
            sz = regsize(val->u.addr.offset);
            if ( 0 != val->u.addr.saddrsize && sz != val->u.addr.saddrsize ) {
                /* Invalid operand size */
                free(val);
                return NULL;
            }
            val->u.addr.saddrsize = sz;
        }
        break;
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

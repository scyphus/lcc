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
#include <string.h>

/*
 * Prototype declarations
 */
#if 0
static x86_64_val_t * _eval_expr_var(const x86_64_label_table_t *, expr_t *);
static x86_64_val_t * _eval_expr_int(const x86_64_label_table_t *, expr_t *);
static x86_64_val_t * _eval_expr_op(const x86_64_label_table_t *, expr_t *);
static x86_64_val_t * _eval_expr(const x86_64_label_table_t *, expr_t *);
#endif

static x86_64_eval_t *
_estimate_expr_var(const x86_64_label_table_t *, expr_t *);
static x86_64_eval_t *
_estimate_expr_int(const x86_64_label_table_t *, expr_t *);
static x86_64_eval_t *
_estimate_expr_op(const x86_64_label_table_t *, expr_t *);
static x86_64_eval_t * _estimate_expr(const x86_64_label_table_t *, expr_t *);

#if 0
/*
 * Evaluate var expression
 */
static x86_64_val_t *
_eval_expr_var(const x86_64_label_table_t *ltbl, expr_t *expr)
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
_eval_expr_int(const x86_64_label_table_t *ltbl, expr_t *expr)
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
_eval_expr_op(const x86_64_label_table_t *ltbl, expr_t *expr)
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
        lval = _eval_expr(ltbl, expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            /* Do nothing */
            val->u.imm = lval->u.imm;
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

        lval = _eval_expr(ltbl, expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        rval = _eval_expr(ltbl, expr1);
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
_eval_expr(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_val_t *val;

    switch ( expr->type ) {
    case EXPR_VAR:
        val = _eval_expr_var(ltbl, expr);
        break;
    case EXPR_INT:
        val = _eval_expr_int(ltbl, expr);
        break;
    case EXPR_OP:
        val = _eval_expr_op(ltbl, expr);
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
_eval_expr_imm_or_reg(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_val_t *val;

    val = _eval_expr(ltbl, expr);

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
_eval_expr_addr(const x86_64_label_table_t *ltbl, pexpr_t *pexpr)
{
    x86_64_val_t *val;
    x86_64_reg_t reg;
    int64_t imm;

    val = _eval_expr(ltbl, pexpr->expr);
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
        val = _eval_expr_imm_or_reg(ltbl, op->op.expr);
    } else {
        /* Address */
        val = _eval_expr_addr(ltbl, op->op.pexpr);
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
#endif










/*
 * Search a label from the table
 */
static x86_64_label_t *
_search_label(const x86_64_label_table_t *tbl, const char *var)
{
    x86_64_label_ent_t *ent;

    ent = tbl->root;
    while ( NULL != ent ) {
        if ( 0 == strcmp(ent->c->label, var) ) {
            /* Set the scope as global */
            return ent->c;
        }
        ent = ent->next;
    }

    return NULL;
}



/*
 * Estimate var expression
 */
static x86_64_eval_t *
_estimate_expr_var(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_eval_t *eval;
    x86_64_reg_t reg;
    x86_64_label_t *lb;

    /* Allocate estimated value */
    eval = malloc(sizeof(x86_64_eval_t));
    if ( NULL == eval ) {
        return NULL;
    }

    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol */
        lb = _search_label(ltbl, expr->u.var);
        if ( NULL == lb ) {
            /* Not found */
            eval->type = X86_64_EVAL_IMM;
            eval->u.imm.type = X86_64_IMM_REL;
            eval->u.imm.u.rexpr = expr;
            eval->sopsize = 0;
        } else {
            eval->type = X86_64_EVAL_IMM;
            eval->u.imm.type = X86_64_IMM_ESTIMATED;
            eval->u.imm.u.est.loff = 1;
            eval->u.imm.u.est.expr = expr;
            eval->u.imm.u.est.min = lb->min;
            eval->u.imm.u.est.max = lb->max;
            eval->sopsize = 0;
        }
    } else {
        /* Register */
        eval->type = X86_64_EVAL_REG;
        eval->u.reg = reg;
        eval->sopsize = 0;
    }

    return eval;
}

/*
 * Estimate integer expression
 */
static x86_64_eval_t *
_estimate_expr_int(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_eval_t *eval;

    eval = malloc(sizeof(x86_64_eval_t));
    if ( NULL == eval ) {
        return NULL;
    }
    eval->type = X86_64_EVAL_IMM;
    eval->u.imm.type = X86_64_IMM_FIXED;
    eval->u.imm.u.fixed = expr->u.i;
    eval->sopsize = 0;

    return eval;
}

/*
 * Estimate operand
 */
static x86_64_eval_t *
_estimate_expr_op(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_eval_t *eval;
    x86_64_eval_t *leval;
    x86_64_eval_t *reval;
    expr_t *expr0;
    expr_t *expr1;

    /* Allocate the estimated value */
    eval = malloc(sizeof(x86_64_eval_t));
    if ( NULL == eval ) {
        return NULL;
    }
    eval->sopsize = 0;

    /* Refactoring is required... */
    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        leval = _estimate_expr(ltbl, expr0);
        if ( NULL == leval ) {
            free(eval);
            return NULL;
        }
        if ( X86_64_EVAL_IMM == leval->type ) {
            if ( X86_64_IMM_FIXED == leval->u.imm.type ) {
                eval->type = X86_64_EVAL_IMM;
                eval->u.imm.type = X86_64_IMM_FIXED;
                switch ( expr->u.op.type ) {
                case OP_PLUS:
                    /* Do nothing: Just copy */
                    eval->u.imm.u.fixed = leval->u.imm.u.fixed;
                    break;
                case OP_MINUS:
                    /* Minus */
                    eval->u.imm.u.fixed = -leval->u.imm.u.fixed;
                    break;
                case OP_TILDE:
                    /* Bitwise not */
                    eval->u.imm.u.fixed = ~leval->u.imm.u.fixed;
                    break;
                default:
                    free(eval);
                    return NULL;
                }
            } else if ( X86_64_IMM_ESTIMATED == leval->u.imm.type ) {
                eval->type = X86_64_EVAL_IMM;
                eval->u.imm.type = X86_64_IMM_ESTIMATED;
                switch ( expr->u.op.type ) {
                case OP_PLUS:
                    /* Do nothing */
                    eval->u.imm.u.est.loff = leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min = leval->u.imm.u.est.max;
                    eval->u.imm.u.est.max = leval->u.imm.u.est.min;
                    break;
                case OP_MINUS:
                    /* Minus */
                    eval->u.imm.u.est.loff = -leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min = -leval->u.imm.u.est.max;
                    eval->u.imm.u.est.max = -leval->u.imm.u.est.min;
                    break;
                default:
                    free(eval);
                    return NULL;
                }
            } else {
                eval->type = X86_64_EVAL_IMM;
                eval->u.imm.type = X86_64_IMM_REL;
                switch ( expr->u.op.type ) {
                case OP_PLUS:
                case OP_MINUS:
                    /* Do nothing */
                    eval->u.imm.u.rexpr = expr;
                    break;
                default:
                    free(eval);
                    return NULL;
                }
            }
        } else {
            free(eval);
            return NULL;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        leval = _estimate_expr(ltbl, expr0);
        if ( NULL == leval ) {
            free(eval);
            return NULL;
        }
        reval = _estimate_expr(ltbl, expr1);
        if ( NULL == reval ) {
            /* Free lval */
            free(eval);
            free(leval);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            if ( X86_64_EVAL_IMM == leval->type
                 && X86_64_EVAL_IMM == reval->type ) {
                if ( X86_64_IMM_FIXED == leval->u.imm.type
                     && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_FIXED;
                    eval->u.imm.u.fixed
                        = leval->u.imm.u.fixed + reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_FIXED == leval->u.imm.type
                            && X86_64_IMM_ESTIMATED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = reval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.fixed + reval->u.imm.u.est.min;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.fixed + reval->u.imm.u.est.max;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_ESTIMATED == leval->u.imm.type
                            && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.est.min + reval->u.imm.u.fixed;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.est.max + reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_ESTIMATED == leval->u.imm.type
                            && X86_64_IMM_ESTIMATED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.est.min + reval->u.imm.u.est.min;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.est.max + reval->u.imm.u.est.max;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_REL == leval->u.imm.type
                            || X86_64_IMM_REL == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_REL;
                    eval->u.imm.u.rexpr = expr;
                    eval->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else if ( X86_64_EVAL_REG == leval->type
                        && X86_64_EVAL_IMM == reval->type ) {
                /* Base register + Displacement */
                eval->type = X86_64_EVAL_ADDR;
                eval->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                eval->u.eaddr.base = leval->u.reg;
                (void)memcpy(&eval->u.eaddr.disp, &reval->u.imm,
                             sizeof(x86_64_imm_t));
                eval->u.eaddr.saddrsize = 0;
                eval->sopsize = 0;
            } else if ( X86_64_EVAL_IMM == leval->type
                        && X86_64_EVAL_REG == reval->type ) {
                /* Base register + Displacement */
                eval->type = X86_64_EVAL_ADDR;
                eval->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                eval->u.eaddr.base = reval->u.reg;
                (void)memcpy(&eval->u.eaddr.disp, &leval->u.imm,
                             sizeof(x86_64_imm_t));
                eval->u.eaddr.saddrsize = 0;
                eval->sopsize = 0;
            } else if ( X86_64_EVAL_ADDR == leval->type ) {
                if ( X86_64_EVAL_REG == reval->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & leval->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(eval);
                        free(leval);
                        free(reval);
                        return NULL;
                    }
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags = X86_64_ADDR_BASE
                        | leval->u.eaddr.flags;
                    eval->u.eaddr.base = reval->u.reg;
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else if ( X86_64_EVAL_IMM == reval->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & leval->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(eval);
                        free(leval);
                        free(reval);
                        return NULL;
                    }
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags = X86_64_ADDR_DISP
                        | leval->u.eaddr.flags;
                    (void)memcpy(&eval->u.eaddr.disp, &reval->u.imm,
                                 sizeof(x86_64_imm_t));
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else if ( X86_64_EVAL_ADDR == reval->type ) {
                if ( X86_64_EVAL_REG == leval->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & reval->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(eval);
                        free(leval);
                        free(reval);
                        return NULL;
                    }
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags = X86_64_ADDR_BASE
                        | reval->u.eaddr.flags;
                    eval->u.eaddr.base = leval->u.reg;
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else if ( X86_64_EVAL_IMM == leval->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & reval->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(eval);
                        free(leval);
                        free(reval);
                        return NULL;
                    }
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags = X86_64_ADDR_DISP
                        | reval->u.eaddr.flags;
                    (void)memcpy(&eval->u.eaddr.disp, &leval->u.imm,
                                 sizeof(x86_64_imm_t));
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(eval);
                free(leval);
                free(reval);
                return NULL;
            }
            break;
        case OP_MINUS:
            if ( X86_64_EVAL_IMM == leval->type
                 && X86_64_EVAL_IMM == reval->type ) {
                if ( X86_64_IMM_FIXED == leval->u.imm.type
                     && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_FIXED;
                    eval->u.imm.u.fixed
                        = leval->u.imm.u.fixed - reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_FIXED == leval->u.imm.type
                            && X86_64_IMM_ESTIMATED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = reval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.fixed - reval->u.imm.u.est.max;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.fixed - reval->u.imm.u.est.min;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_ESTIMATED == leval->u.imm.type
                            && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.est.min - reval->u.imm.u.fixed;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.est.max - reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_ESTIMATED == leval->u.imm.type
                            && X86_64_IMM_ESTIMATED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_ESTIMATED;
                    eval->u.imm.u.est.loff = leval->u.imm.u.est.loff;
                    eval->u.imm.u.est.expr = expr;
                    eval->u.imm.u.est.min
                        = leval->u.imm.u.est.min - reval->u.imm.u.est.max;
                    eval->u.imm.u.est.max
                        = leval->u.imm.u.est.max - reval->u.imm.u.est.min;
                    eval->sopsize = 0;
                } else if ( X86_64_IMM_REL == leval->u.imm.type
                            || X86_64_IMM_REL == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_REL;
                    eval->u.imm.u.rexpr = expr;
                    eval->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(eval);
                free(leval);
                free(reval);
                return NULL;
            }
            break;
        case OP_MUL:
            if ( X86_64_EVAL_IMM == leval->type
                 && X86_64_EVAL_IMM == reval->type ) {
                if ( X86_64_IMM_FIXED == leval->u.imm.type
                     && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_FIXED;
                    eval->u.imm.u.fixed
                        = leval->u.imm.u.fixed * reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else {
                    /* Invalid */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else if ( X86_64_EVAL_IMM == leval->type
                        && X86_64_EVAL_REG == reval->type ) {
                if ( X86_64_IMM_FIXED == leval->u.imm.type ) {
                    /* The scale must be a fixed value */
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags
                        = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    eval->u.eaddr.offset = reval->u.reg;
                    eval->u.eaddr.scale = leval->u.imm.u.fixed;
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else {
                    /* Invalid */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else if ( X86_64_EVAL_REG == leval->type
                        && X86_64_EVAL_IMM == reval->type ) {
                if ( X86_64_IMM_FIXED == reval->u.imm.type ) {
                    /* The scale must be a fixed value */
                    eval->type = X86_64_EVAL_ADDR;
                    eval->u.eaddr.flags
                        = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    eval->u.eaddr.offset = leval->u.reg;
                    eval->u.eaddr.scale = reval->u.imm.u.fixed;
                    eval->u.eaddr.saddrsize = 0;
                    eval->sopsize = 0;
                } else {
                    /* Invalid */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(eval);
                free(leval);
                free(reval);
                return NULL;
            }
            break;
        case OP_DIV:
            if ( X86_64_EVAL_IMM == leval->type
                 && X86_64_EVAL_IMM == reval->type ) {
                if ( X86_64_IMM_FIXED == leval->u.imm.type
                     && X86_64_IMM_FIXED == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_FIXED;
                    eval->u.imm.u.fixed
                        = leval->u.imm.u.fixed / reval->u.imm.u.fixed;
                    eval->sopsize = 0;
                } else {
                    /* Invalid */
                    free(eval);
                    free(leval);
                    free(reval);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(eval);
                free(leval);
                free(reval);
                return NULL;
            }
            break;
        default:
            /* Unsupported operator */
            free(eval);
            free(leval);
            free(reval);
            return NULL;
        }

    }

    return eval;
}

/*
 * Estimate the expression (static function)
 */
static x86_64_eval_t *
_estimate_expr(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_eval_t *eval;

    switch ( expr->type ) {
    case EXPR_VAR:
        eval = _estimate_expr_var(ltbl, expr);
        break;
    case EXPR_INT:
        eval = _estimate_expr_int(ltbl, expr);
        break;
    case EXPR_OP:
        eval = _estimate_expr_op(ltbl, expr);
        break;
    default:
        eval = NULL;
    }

    return eval;
}

/*
 * Estimate immediate value or register
 */
static x86_64_eval_t *
_estimate_expr_imm_or_reg(const x86_64_label_table_t *ltbl, expr_t *expr)
{
    x86_64_eval_t *eval;

    eval = _estimate_expr(ltbl, expr);

    /* Verify the returned value */
    if ( NULL == eval ) {
        return NULL;
    } else if ( X86_64_EVAL_REG != eval->type
                && X86_64_EVAL_IMM != eval->type ) {
        free(eval);
        return NULL;
    }

    return eval;
}

/*
 * Estimate the expression which is address operand type
 */
static x86_64_eval_t *
_estimate_expr_addr(const x86_64_label_table_t *ltbl, pexpr_t *pexpr)
{
    x86_64_eval_t *eval;
    x86_64_reg_t reg;
    x86_64_imm_t imm;

    eval = _estimate_expr(ltbl, pexpr->expr);
    if ( NULL == eval ) {
        return NULL;
    }

    /* Convert the type to address operand */
    if ( X86_64_EVAL_REG == eval->type ) {
        reg = eval->u.reg;
        eval->type = X86_64_EVAL_ADDR;
        eval->u.eaddr.flags = X86_64_ADDR_BASE;
        eval->u.eaddr.base = reg;
        eval->u.eaddr.saddrsize = eval->sopsize;
        eval->sopsize = 0;
    } else if ( X86_64_EVAL_IMM == eval->type ) {
        (void)memcpy(&imm, &eval->u.imm, sizeof(x86_64_imm_t));
        eval->type = X86_64_EVAL_ADDR;
        eval->u.eaddr.flags = X86_64_ADDR_DISP;
        (void)memcpy(&eval->u.eaddr.disp, &imm, sizeof(x86_64_imm_t));
        eval->u.eaddr.saddrsize = eval->sopsize;
        eval->sopsize = 0;
    }

    /* Check the operand prefix */
    switch ( pexpr->prefix ) {
    case SIZE_PREFIX_BYTE:
        eval->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        eval->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        eval->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        eval->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Verify the returned value */
    if ( X86_64_EVAL_ADDR != eval->type ) {
        free(eval);
        return NULL;
    }

    return eval;
}

/*
 * Estimate the operand
 */
x86_64_eval_t *
x86_64_estimate_operand(const x86_64_label_table_t *ltbl, operand_t *op)
{
    x86_64_eval_t *eval;
    size_t sz;

    if ( OPERAND_EXPR == op->type ) {
        /* Immediate value or register */
        eval = _estimate_expr_imm_or_reg(ltbl, op->op.expr);
    } else {
        /* Address */
        eval = _estimate_expr_addr(ltbl, op->op.pexpr);
    }
    /* Check the returned value */
    if ( NULL == eval ) {
        return NULL;
    }

    /* Check the operand prefix */
    switch ( op->prefix ) {
    case SIZE_PREFIX_BYTE:
        eval->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        eval->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        eval->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        eval->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Complement and validate the operand size and address size */
    switch ( eval->type ) {
    case X86_64_EVAL_IMM:
        /* Check nothing */
        break;
    case X86_64_EVAL_REG:
        sz = regsize(eval->u.reg);
        if ( 0 != eval->sopsize && sz != eval->sopsize ) {
            /* Invalid operand size */
            free(eval);
            return NULL;
        }
        eval->sopsize = sz;
        break;
    case X86_64_EVAL_ADDR:
        if ( X86_64_ADDR_BASE & eval->u.eaddr.flags ) {
            sz = regsize(eval->u.eaddr.base);
            if ( 0 != eval->u.eaddr.saddrsize
                 && sz != eval->u.eaddr.saddrsize ) {
                /* Invalid operand size */
                free(eval);
                return NULL;
            }
            eval->u.eaddr.saddrsize = sz;
        }
        if ( X86_64_ADDR_OFFSET & eval->u.eaddr.flags ) {
            sz = regsize(eval->u.eaddr.offset);
            if ( 0 != eval->u.eaddr.saddrsize
                 && sz != eval->u.eaddr.saddrsize ) {
                /* Invalid operand size */
                free(eval);
                return NULL;
            }
            eval->u.eaddr.saddrsize = sz;
        }
        break;
    }

    return eval;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

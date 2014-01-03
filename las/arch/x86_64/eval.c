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
static x86_64_rval_t *
_expr_fix_var(const x86_64_label_table_t *, const expr_t *);
static x86_64_rval_t *
_expr_fix_int(const x86_64_label_table_t *, const expr_t *);
static x86_64_rval_t *
_expr_fix_op(const x86_64_label_table_t *, const expr_t *);
static x86_64_rval_t * _expr_fix(const x86_64_label_table_t *, const expr_t *);


static int
_expr_range_var(const x86_64_label_table_t *, const expr_t *, int64_t *,
                int64_t *);
static int
_expr_range_int(const x86_64_label_table_t *, const expr_t *, int64_t *,
                int64_t *);
static int
_expr_range_op(const x86_64_label_table_t *, const expr_t *, int64_t *,
               int64_t *);
static int
_expr_range(const x86_64_label_table_t *, const expr_t *, int64_t *, int64_t *);

static x86_64_eval_t * _estimate_expr_var(expr_t *);
static x86_64_eval_t * _estimate_expr_int(expr_t *);
static x86_64_eval_t * _estimate_expr_op(expr_t *);
static x86_64_eval_t * _estimate_expr(expr_t *);




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
 * Evaluate var expression
 */
static x86_64_rval_t *
_expr_fix_var(const x86_64_label_table_t *ltbl, const expr_t *expr)
{
    x86_64_rval_t *rval;
    x86_64_reg_t reg;
    x86_64_label_t *lb;

    /* Allocate a relocatable value */
    rval = malloc(sizeof(x86_64_rval_t));
    if ( NULL == rval ) {
        return NULL;
    }
    rval->nsym = 1;
    rval->addend = 0;
    rval->symname = NULL;

    /* Check whether it is a register */
    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN != reg ) {
        /* Must not be a register */
        free(rval);
        return NULL;
    }

    /* Search the corresponding label */
    lb = _search_label(ltbl, expr->u.var);
    if ( NULL == lb ) {
        /* Not found: to be linked */
        rval->symname = strdup(expr->u.var);
        if ( NULL == rval->symname ) {
            free(rval);
            return NULL;
        }

        return rval;
    } else {
        /* Found */
        /* FIXME: Replace "min" with the fixed position  */
        rval->addend = lb->min;

        return rval;
    }
}

/*
 * Evaluate int expression
 */
static x86_64_rval_t *
_expr_fix_int(const x86_64_label_table_t *ltbl, const expr_t *expr)
{
    x86_64_rval_t *rval;

    /* Allocate a relocatable value */
    rval = malloc(sizeof(x86_64_rval_t));
    if ( NULL == rval ) {
        return NULL;
    }
    rval->nsym = 0;
    rval->addend = expr->u.i;
    rval->symname = NULL;

    return rval;
}

/*
 * Evaluate op expression
 */
static x86_64_rval_t *
_expr_fix_op(const x86_64_label_table_t *ltbl, const expr_t *expr)
{
    x86_64_rval_t *rval;
    x86_64_rval_t *lrval;
    x86_64_rval_t *rrval;
    expr_t *expr0;
    expr_t *expr1;

    rval = NULL;
    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        lrval = _expr_fix(ltbl, expr0);
        if ( NULL == lrval ) {
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            /* Do nothing */
            rval = lrval;
            break;
        case OP_MINUS:
            /* Minus */
            if ( NULL == lrval->symname ) {
                /* Jist copy it */
                rval = lrval;
            } else {
                /* Invalid operation */
                free(lrval->symname);
                free(lrval);
                return NULL;
            }
            break;
        default:
            return NULL;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        lrval = _expr_fix(ltbl, expr0);
        if ( NULL == lrval ) {
            return NULL;
        }
        rrval = _expr_fix(ltbl, expr1);
        if ( NULL == rrval ) {
            if ( NULL != rrval->symname ) {
                free(rrval->symname);
            }
            free(rrval);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            if ( lrval->symname && rrval->symname ) {
                /* Cannot have two symbols */
                rval = NULL;
            } else if ( lrval->symname ) {
                rval = malloc(sizeof(x86_64_rval_t));
                if ( NULL != rval ) {
                    rval->symname = strdup(lrval->symname);
                    if ( NULL == rval->symname ) {
                        free(rval);
                        rval = NULL;
                    } else {
                        rval->nsym = lrval->nsym + rrval->nsym;
                        rval->addend = lrval->addend + rrval->addend;
                    }
                }
            } else if ( rrval->symname ) {
                rval = malloc(sizeof(x86_64_rval_t));
                if ( NULL != rval ) {
                    rval->symname = strdup(rrval->symname);
                    if ( NULL == rval->symname ) {
                        free(rval);
                        rval = NULL;
                    } else {
                        rval->nsym = lrval->nsym + rrval->nsym;
                        rval->addend = lrval->addend + rrval->addend;
                    }
                }
            } else {
                rval = malloc(sizeof(x86_64_rval_t));
                if ( NULL != rval ) {
                    rval->symname = NULL;
                    rval->nsym = lrval->nsym + rrval->nsym;
                    rval->addend = lrval->addend + rrval->addend;
                }
            }
            break;
        case OP_MINUS:
            if ( rrval->symname ) {
                /* Cannot have a minus symbol value */
                rval = NULL;
            } else if ( lrval->symname ) {
                rval = malloc(sizeof(x86_64_rval_t));
                if ( NULL != rval ) {
                    rval->symname = strdup(lrval->symname);
                    if ( NULL == rval->symname ) {
                        free(rval);
                        rval = NULL;
                    } else {
                        rval->nsym = lrval->nsym - rrval->nsym;
                        rval->addend = lrval->addend - rrval->addend;
                    }
                }
            } else {
                rval = malloc(sizeof(x86_64_rval_t));
                if ( NULL != rval ) {
                    rval->symname = NULL;
                    rval->nsym = lrval->nsym - rrval->nsym;
                    rval->addend = lrval->addend - rrval->addend;
                }
            }
            break;
        default:
            rval = NULL;
        }

        /* Free */
        if ( NULL == lrval->symname ) {
            free(lrval->symname);
        }
        free(lrval);
        if ( NULL == rrval->symname ) {
            free(rrval->symname);
        }
        free(rrval);
    }

    return rval;
}

/*
 * Evaluate the expression (static function)
 */
static x86_64_rval_t *
_expr_fix(const x86_64_label_table_t *ltbl, const expr_t *expr)
{
    x86_64_rval_t *rval;

    switch ( expr->type ) {
    case EXPR_VAR:
        rval = _expr_fix_var(ltbl, expr);
        break;
    case EXPR_INT:
        rval = _expr_fix_int(ltbl, expr);
        break;
    case EXPR_OP:
        rval = _expr_fix_op(ltbl, expr);
        break;
    default:
        rval = NULL;
    }

    return rval;
}

/*
 * Evaluate the expression
 */
x86_64_rval_t *
x86_64_expr_fix(const x86_64_label_table_t *ltbl, const expr_t *expr)
{
    return _expr_fix(ltbl, expr);
}



/*
 * Evaluate var expression
 */
static int
_expr_range_var(const x86_64_label_table_t *ltbl, const expr_t *expr,
                int64_t *min, int64_t *max)
{
    x86_64_reg_t reg;
    x86_64_label_t *lb;

    /* Check whether it is a register */
    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN != reg ) {
        /* Must not be a register */
        return -1;
    }

    /* Search the corresponding label */
    lb = _search_label(ltbl, expr->u.var);
    if ( NULL == lb ) {
        /* Not found: to be linked */
        *min = X86_64_VAR_MIN;
        *max = X86_64_VAR_MAX;

        return 0;
    } else {
        /* Found */
        *min = lb->min;
        *max = lb->max;

        return 1;
    }
}

/*
 * Evaluate var expression
 */
static int
_expr_range_int(const x86_64_label_table_t *ltbl, const expr_t *expr,
                int64_t *min, int64_t *max)
{
    /* Found */
    *min = expr->u.i;
    *max = expr->u.i;

    return 1;
}

/*
 * Evaluate op expression
 */
static int
_expr_range_op(const x86_64_label_table_t *ltbl, const expr_t *expr,
                int64_t *min, int64_t *max)
{
    int ret;
    int64_t lmin;
    int64_t lmax;
    int64_t rmin;
    int64_t rmax;
    expr_t *expr0;
    expr_t *expr1;

    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        ret = _expr_range(ltbl, expr0, &lmin, &lmax);
        if ( ret < 0 ) {
            return -1;
        } else if ( 0 == ret ) {
            /* Just copy */
            *min = lmin;
            *max = lmax;
            return 0;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            /* Do nothing */
            *min = lmin;
            *max = lmax;
            break;
        case OP_MINUS:
            /* Minus */
            *min = -lmax;
            *max = -lmin;
            break;
        default:
            return -1;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        ret = _expr_range(ltbl, expr0, &lmin, &lmax);
        if ( ret < 0 ) {
            return -1;
        } else if ( 0 == ret ) {
            /* Just copy */
            *min = lmin;
            *max = lmax;
            return 0;
        }
        ret = _expr_range(ltbl, expr1, &rmin, &rmax);
        if ( ret < 0 ) {
            return -1;
        } else if ( 0 == ret ) {
            /* Just copy */
            *min = lmin;
            *max = lmax;
            return 0;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            *min = lmin + rmin;
            *max = lmax + rmax;
            break;
        case OP_MINUS:
            *min = lmin - rmax;
            *max = lmax - rmin;
            break;
        default:
            return -1;
        }
    } else {
        return -1;
    }

    return 1;
}

/*
 * Evaluate the expression (static function)
 */
static int
_expr_range(const x86_64_label_table_t *ltbl, const expr_t *expr, int64_t *min,
            int64_t *max)
{
    int ret;

    switch ( expr->type ) {
    case EXPR_VAR:
        ret = _expr_range_var(ltbl, expr, min, max);
        break;
    case EXPR_INT:
        ret = _expr_range_int(ltbl, expr, min, max);
        break;
    case EXPR_OP:
        ret = _expr_range_op(ltbl, expr, min, max);
        break;
    default:
        ret = -1;
    }

    return ret;
}

/*
 * Evaluate the expression
 */
int
x86_64_expr_range(const x86_64_label_table_t *ltbl, const expr_t *expr,
                  int64_t *min, int64_t *max)
{
    return _expr_range(ltbl, expr, min, max);
}













/*
 * Estimate var expression
 */
static x86_64_eval_t *
_estimate_expr_var(expr_t *expr)
{
    x86_64_eval_t *eval;
    x86_64_reg_t reg;

    /* Allocate estimated value */
    eval = malloc(sizeof(x86_64_eval_t));
    if ( NULL == eval ) {
        return NULL;
    }

    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol */
        eval->type = X86_64_EVAL_IMM;
        eval->u.imm.type = X86_64_IMM_EXPR;
        eval->u.imm.u.rexpr = expr;
        eval->sopsize = 0;
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
_estimate_expr_int(expr_t *expr)
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
_estimate_expr_op(expr_t *expr)
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
        leval = _estimate_expr(expr0);
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
            } else {
                eval->type = X86_64_EVAL_IMM;
                eval->u.imm.type = X86_64_IMM_EXPR;
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

        leval = _estimate_expr(expr0);
        if ( NULL == leval ) {
            free(eval);
            return NULL;
        }
        reval = _estimate_expr(expr1);
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
                } else if ( X86_64_IMM_EXPR == leval->u.imm.type
                            || X86_64_IMM_EXPR == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_EXPR;
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
            } else if ( X86_64_EVAL_REG == leval->type
                        && X86_64_EVAL_REG == reval->type ) {
                /* Base register + Offset register */
                eval->type = X86_64_EVAL_ADDR;
                eval->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_OFFSET
                    | X86_64_ADDR_SCALE;
                eval->u.eaddr.base = leval->u.reg;
                eval->u.eaddr.offset = reval->u.reg;
                eval->u.eaddr.scale = 1;
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
                    eval->u.eaddr.offset = leval->u.eaddr.offset;
                    eval->u.eaddr.scale = leval->u.eaddr.scale;
                    (void)memcpy(&eval->u.eaddr.disp, &leval->u.eaddr.disp,
                                 sizeof(x86_64_imm_t));
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
                    eval->u.eaddr.base = leval->u.eaddr.base;
                    eval->u.eaddr.offset = leval->u.eaddr.offset;
                    eval->u.eaddr.scale = leval->u.eaddr.scale;
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
                } else if ( X86_64_IMM_EXPR == leval->u.imm.type
                            || X86_64_IMM_EXPR == reval->u.imm.type ) {
                    eval->type = X86_64_EVAL_IMM;
                    eval->u.imm.type = X86_64_IMM_EXPR;
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
_estimate_expr(expr_t *expr)
{
    x86_64_eval_t *eval;

    switch ( expr->type ) {
    case EXPR_VAR:
        eval = _estimate_expr_var(expr);
        break;
    case EXPR_INT:
        eval = _estimate_expr_int(expr);
        break;
    case EXPR_OP:
        eval = _estimate_expr_op(expr);
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
_estimate_expr_imm_or_reg(expr_t *expr)
{
    x86_64_eval_t *eval;

    eval = _estimate_expr(expr);

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
_estimate_expr_addr(pexpr_t *pexpr)
{
    x86_64_eval_t *eval;
    x86_64_reg_t reg;
    x86_64_imm_t imm;

    eval = _estimate_expr(pexpr->expr);
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
x86_64_estimate_operand(operand_t *op)
{
    x86_64_eval_t *eval;
    size_t sz;

    if ( OPERAND_EXPR == op->type ) {
        /* Immediate value or register */
        eval = _estimate_expr_imm_or_reg(op->op.expr);
    } else {
        /* Address */
        eval = _estimate_expr_addr(op->op.pexpr);
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

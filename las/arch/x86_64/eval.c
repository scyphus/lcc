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

static x86_64_opr_t * _estimate_expr_var(expr_t *);
static x86_64_opr_t * _estimate_expr_int(expr_t *);
static x86_64_opr_t * _estimate_expr_op(expr_t *);
static x86_64_opr_t * _estimate_expr(expr_t *);




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
static x86_64_opr_t *
_estimate_expr_var(expr_t *expr)
{
    x86_64_opr_t *opr;
    x86_64_reg_t reg;

    /* Allocate estimated value */
    opr = malloc(sizeof(x86_64_opr_t));
    if ( NULL == opr ) {
        return NULL;
    }

    reg = strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol */
        opr->type = X86_64_OPR_IMM;
        opr->u.imm.type = X86_64_IMM_EXPR;
        opr->u.imm.u.rexpr = expr;
        opr->sopsize = 0;
    } else {
        /* Register */
        opr->type = X86_64_OPR_REG;
        opr->u.reg = reg;
        opr->sopsize = 0;
    }

    return opr;
}

/*
 * Estimate integer expression
 */
static x86_64_opr_t *
_estimate_expr_int(expr_t *expr)
{
    x86_64_opr_t *opr;

    opr = malloc(sizeof(x86_64_opr_t));
    if ( NULL == opr ) {
        return NULL;
    }
    opr->type = X86_64_OPR_IMM;
    opr->u.imm.type = X86_64_IMM_FIXED;
    opr->u.imm.u.fixed = expr->u.i;
    opr->sopsize = 0;

    return opr;
}

/*
 * Estimate operand
 */
static x86_64_opr_t *
_estimate_expr_op(expr_t *expr)
{
    x86_64_opr_t *opr;
    x86_64_opr_t *lopr;
    x86_64_opr_t *ropr;
    expr_t *expr0;
    expr_t *expr1;

    /* Allocate the estimated value */
    opr = malloc(sizeof(x86_64_opr_t));
    if ( NULL == opr ) {
        return NULL;
    }
    opr->sopsize = 0;

    /* Refactoring is required... */
    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        lopr = _estimate_expr(expr0);
        if ( NULL == lopr ) {
            free(opr);
            return NULL;
        }
        if ( X86_64_OPR_IMM == lopr->type ) {
            if ( X86_64_IMM_FIXED == lopr->u.imm.type ) {
                opr->type = X86_64_OPR_IMM;
                opr->u.imm.type = X86_64_IMM_FIXED;
                switch ( expr->u.op.type ) {
                case OP_PLUS:
                    /* Do nothing: Just copy */
                    opr->u.imm.u.fixed = lopr->u.imm.u.fixed;
                    break;
                case OP_MINUS:
                    /* Minus */
                    opr->u.imm.u.fixed = -lopr->u.imm.u.fixed;
                    break;
                case OP_TILDE:
                    /* Bitwise not */
                    opr->u.imm.u.fixed = ~lopr->u.imm.u.fixed;
                    break;
                default:
                    free(opr);
                    return NULL;
                }
            } else {
                opr->type = X86_64_OPR_IMM;
                opr->u.imm.type = X86_64_IMM_EXPR;
                switch ( expr->u.op.type ) {
                case OP_PLUS:
                case OP_MINUS:
                    /* Do nothing */
                    opr->u.imm.u.rexpr = expr;
                    break;
                default:
                    free(opr);
                    return NULL;
                }
            }
        } else {
            free(opr);
            return NULL;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        lopr = _estimate_expr(expr0);
        if ( NULL == lopr ) {
            free(opr);
            return NULL;
        }
        ropr = _estimate_expr(expr1);
        if ( NULL == ropr ) {
            /* Free lval */
            free(opr);
            free(lopr);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            if ( X86_64_OPR_IMM == lopr->type
                 && X86_64_OPR_IMM == ropr->type ) {
                if ( X86_64_IMM_FIXED == lopr->u.imm.type
                     && X86_64_IMM_FIXED == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_FIXED;
                    opr->u.imm.u.fixed
                        = lopr->u.imm.u.fixed + ropr->u.imm.u.fixed;
                    opr->sopsize = 0;
                } else if ( X86_64_IMM_EXPR == lopr->u.imm.type
                            || X86_64_IMM_EXPR == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_EXPR;
                    opr->u.imm.u.rexpr = expr;
                    opr->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else if ( X86_64_OPR_REG == lopr->type
                        && X86_64_OPR_IMM == ropr->type ) {
                /* Base register + Displacement */
                opr->type = X86_64_OPR_ADDR;
                opr->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                opr->u.eaddr.base = lopr->u.reg;
                (void)memcpy(&opr->u.eaddr.disp, &ropr->u.imm,
                             sizeof(x86_64_imm_t));
                opr->u.eaddr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_IMM == lopr->type
                        && X86_64_OPR_REG == ropr->type ) {
                /* Base register + Displacement */
                opr->type = X86_64_OPR_ADDR;
                opr->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                opr->u.eaddr.base = ropr->u.reg;
                (void)memcpy(&opr->u.eaddr.disp, &lopr->u.imm,
                             sizeof(x86_64_imm_t));
                opr->u.eaddr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_REG == lopr->type
                        && X86_64_OPR_REG == ropr->type ) {
                /* Base register + Offset register */
                opr->type = X86_64_OPR_ADDR;
                opr->u.eaddr.flags = X86_64_ADDR_BASE | X86_64_ADDR_OFFSET
                    | X86_64_ADDR_SCALE;
                opr->u.eaddr.base = lopr->u.reg;
                opr->u.eaddr.offset = ropr->u.reg;
                opr->u.eaddr.scale = 1;
                opr->u.eaddr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_ADDR == lopr->type ) {
                if ( X86_64_OPR_REG == ropr->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & lopr->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags = X86_64_ADDR_BASE
                        | lopr->u.eaddr.flags;
                    opr->u.eaddr.base = ropr->u.reg;
                    opr->u.eaddr.offset = lopr->u.eaddr.offset;
                    opr->u.eaddr.scale = lopr->u.eaddr.scale;
                    (void)memcpy(&opr->u.eaddr.disp, &lopr->u.eaddr.disp,
                                 sizeof(x86_64_imm_t));
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else if ( X86_64_OPR_IMM == ropr->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & lopr->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags = X86_64_ADDR_DISP
                        | lopr->u.eaddr.flags;
                    (void)memcpy(&opr->u.eaddr.disp, &ropr->u.imm,
                                 sizeof(x86_64_imm_t));
                    opr->u.eaddr.base = lopr->u.eaddr.base;
                    opr->u.eaddr.offset = lopr->u.eaddr.offset;
                    opr->u.eaddr.scale = lopr->u.eaddr.scale;
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else if ( X86_64_OPR_ADDR == ropr->type ) {
                if ( X86_64_OPR_REG == lopr->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & ropr->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags = X86_64_ADDR_BASE
                        | ropr->u.eaddr.flags;
                    opr->u.eaddr.base = lopr->u.reg;
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else if ( X86_64_OPR_IMM == lopr->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & ropr->u.eaddr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags = X86_64_ADDR_DISP
                        | ropr->u.eaddr.flags;
                    (void)memcpy(&opr->u.eaddr.disp, &lopr->u.imm,
                                 sizeof(x86_64_imm_t));
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(opr);
                free(lopr);
                free(ropr);
                return NULL;
            }
            break;
        case OP_MINUS:
            if ( X86_64_OPR_IMM == lopr->type
                 && X86_64_OPR_IMM == ropr->type ) {
                if ( X86_64_IMM_FIXED == lopr->u.imm.type
                     && X86_64_IMM_FIXED == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_FIXED;
                    opr->u.imm.u.fixed
                        = lopr->u.imm.u.fixed - ropr->u.imm.u.fixed;
                    opr->sopsize = 0;
                } else if ( X86_64_IMM_EXPR == lopr->u.imm.type
                            || X86_64_IMM_EXPR == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_EXPR;
                    opr->u.imm.u.rexpr = expr;
                    opr->sopsize = 0;
                } else {
                    /* Invalid syntax */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(opr);
                free(lopr);
                free(ropr);
                return NULL;
            }
            break;
        case OP_MUL:
            if ( X86_64_OPR_IMM == lopr->type
                 && X86_64_OPR_IMM == ropr->type ) {
                if ( X86_64_IMM_FIXED == lopr->u.imm.type
                     && X86_64_IMM_FIXED == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_FIXED;
                    opr->u.imm.u.fixed
                        = lopr->u.imm.u.fixed * ropr->u.imm.u.fixed;
                    opr->sopsize = 0;
                } else {
                    /* Invalid */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else if ( X86_64_OPR_IMM == lopr->type
                        && X86_64_OPR_REG == ropr->type ) {
                if ( X86_64_IMM_FIXED == lopr->u.imm.type ) {
                    /* The scale must be a fixed value */
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags
                        = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    opr->u.eaddr.offset = ropr->u.reg;
                    opr->u.eaddr.scale = lopr->u.imm.u.fixed;
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else {
                    /* Invalid */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else if ( X86_64_OPR_REG == lopr->type
                        && X86_64_OPR_IMM == ropr->type ) {
                if ( X86_64_IMM_FIXED == ropr->u.imm.type ) {
                    /* The scale must be a fixed value */
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.eaddr.flags
                        = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    opr->u.eaddr.offset = lopr->u.reg;
                    opr->u.eaddr.scale = ropr->u.imm.u.fixed;
                    opr->u.eaddr.saddrsize = 0;
                    opr->sopsize = 0;
                } else {
                    /* Invalid */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(opr);
                free(lopr);
                free(ropr);
                return NULL;
            }
            break;
        case OP_DIV:
            if ( X86_64_OPR_IMM == lopr->type
                 && X86_64_OPR_IMM == ropr->type ) {
                if ( X86_64_IMM_FIXED == lopr->u.imm.type
                     && X86_64_IMM_FIXED == ropr->u.imm.type ) {
                    opr->type = X86_64_OPR_IMM;
                    opr->u.imm.type = X86_64_IMM_FIXED;
                    opr->u.imm.u.fixed
                        = lopr->u.imm.u.fixed / ropr->u.imm.u.fixed;
                    opr->sopsize = 0;
                } else {
                    /* Invalid */
                    free(opr);
                    free(lopr);
                    free(ropr);
                    return NULL;
                }
            } else {
                /* Invalid */
                free(opr);
                free(lopr);
                free(ropr);
                return NULL;
            }
            break;
        default:
            /* Unsupported operator */
            free(opr);
            free(lopr);
            free(ropr);
            return NULL;
        }

    }

    return opr;
}

/*
 * Estimate the expression (static function)
 */
static x86_64_opr_t *
_estimate_expr(expr_t *expr)
{
    x86_64_opr_t *opr;

    switch ( expr->type ) {
    case EXPR_VAR:
        opr = _estimate_expr_var(expr);
        break;
    case EXPR_INT:
        opr = _estimate_expr_int(expr);
        break;
    case EXPR_OP:
        opr = _estimate_expr_op(expr);
        break;
    default:
        opr = NULL;
    }

    return opr;
}

/*
 * Estimate immediate value or register
 */
static x86_64_opr_t *
_estimate_expr_imm_or_reg(expr_t *expr)
{
    x86_64_opr_t *opr;

    opr = _estimate_expr(expr);

    /* Verify the returned value */
    if ( NULL == opr ) {
        return NULL;
    } else if ( X86_64_OPR_REG != opr->type
                && X86_64_OPR_IMM != opr->type ) {
        free(opr);
        return NULL;
    }

    return opr;
}

/*
 * Estimate the expression which is address operand type
 */
static x86_64_opr_t *
_estimate_expr_addr(pexpr_t *pexpr)
{
    x86_64_opr_t *opr;
    x86_64_reg_t reg;
    x86_64_imm_t imm;

    opr = _estimate_expr(pexpr->expr);
    if ( NULL == opr ) {
        return NULL;
    }

    /* Convert the type to address operand */
    if ( X86_64_OPR_REG == opr->type ) {
        reg = opr->u.reg;
        opr->type = X86_64_OPR_ADDR;
        opr->u.eaddr.flags = X86_64_ADDR_BASE;
        opr->u.eaddr.base = reg;
        opr->u.eaddr.saddrsize = opr->sopsize;
        opr->sopsize = 0;
    } else if ( X86_64_OPR_IMM == opr->type ) {
        (void)memcpy(&imm, &opr->u.imm, sizeof(x86_64_imm_t));
        opr->type = X86_64_OPR_ADDR;
        opr->u.eaddr.flags = X86_64_ADDR_DISP;
        (void)memcpy(&opr->u.eaddr.disp, &imm, sizeof(x86_64_imm_t));
        opr->u.eaddr.saddrsize = opr->sopsize;
        opr->sopsize = 0;
    }

    /* Check the operand prefix */
    switch ( pexpr->prefix ) {
    case SIZE_PREFIX_BYTE:
        opr->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        opr->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        opr->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        opr->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Verify the returned value */
    if ( X86_64_OPR_ADDR != opr->type ) {
        free(opr);
        return NULL;
    }

    return opr;
}

/*
 * Estimate the operand
 */
x86_64_opr_t *
x86_64_estimate_operand(operand_t *op)
{
    x86_64_opr_t *opr;
    size_t sz;

    if ( OEXPR_EXPR == op->oexpr0->type ) {
        /* Immediate value or register */
        opr = _estimate_expr_imm_or_reg(op->oexpr0->u.expr);
    } else if ( OEXPR_ADDR == op->oexpr0->type ) {
        /* Address */
        opr = _estimate_expr_addr(op->oexpr0->u.pexpr);
    } else {
        return NULL;
    }
    /* Check the returned value */
    if ( NULL == opr ) {
        return NULL;
    }

    /* Check the operand prefix */
    switch ( op->prefix ) {
    case SIZE_PREFIX_BYTE:
        opr->sopsize = SIZE8;
        break;
    case SIZE_PREFIX_WORD:
        opr->sopsize = SIZE16;
        break;
    case SIZE_PREFIX_DWORD:
        opr->sopsize = SIZE32;
        break;
    case SIZE_PREFIX_QWORD:
        opr->sopsize = SIZE64;
        break;
    case SIZE_PREFIX_NONE:
    default:
        ;
    }

    /* Complement and validate the operand size and address size */
    switch ( opr->type ) {
    case X86_64_OPR_IMM:
    case X86_64_OPR_PTR:
        /* Check nothing */
        break;
    case X86_64_OPR_REG:
        sz = regsize(opr->u.reg);
        if ( 0 != opr->sopsize && sz != opr->sopsize ) {
            /* Invalid operand size */
            free(opr);
            return NULL;
        }
        opr->sopsize = sz;
        break;
    case X86_64_OPR_ADDR:
        if ( X86_64_ADDR_BASE & opr->u.eaddr.flags ) {
            sz = regsize(opr->u.eaddr.base);
            if ( 0 != opr->u.eaddr.saddrsize
                 && sz != opr->u.eaddr.saddrsize ) {
                /* Invalid operand size */
                free(opr);
                return NULL;
            }
            opr->u.eaddr.saddrsize = sz;
        }
        if ( X86_64_ADDR_OFFSET & opr->u.eaddr.flags ) {
            sz = regsize(opr->u.eaddr.offset);
            if ( 0 != opr->u.eaddr.saddrsize
                 && sz != opr->u.eaddr.saddrsize ) {
                /* Invalid operand size */
                free(opr);
                return NULL;
            }
            opr->u.eaddr.saddrsize = sz;
        }
        break;
    }

    return opr;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

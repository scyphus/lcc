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
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

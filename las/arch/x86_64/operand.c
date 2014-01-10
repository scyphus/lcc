/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "reg.h"
#include "main.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static x86_64_opr_t * _estimate_expr_var(expr_t *);
static x86_64_opr_t * _estimate_expr_int(expr_t *);
static x86_64_opr_t * _estimate_expr_op(expr_t *);
static x86_64_opr_t * _estimate_expr(expr_t *);

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
                opr->u.addr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                opr->u.addr.base = lopr->u.reg;
                (void)memcpy(&opr->u.addr.disp, &ropr->u.imm,
                             sizeof(x86_64_imm_t));
                opr->u.addr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_IMM == lopr->type
                        && X86_64_OPR_REG == ropr->type ) {
                /* Base register + Displacement */
                opr->type = X86_64_OPR_ADDR;
                opr->u.addr.flags = X86_64_ADDR_BASE | X86_64_ADDR_DISP;
                opr->u.addr.base = ropr->u.reg;
                (void)memcpy(&opr->u.addr.disp, &lopr->u.imm,
                             sizeof(x86_64_imm_t));
                opr->u.addr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_REG == lopr->type
                        && X86_64_OPR_REG == ropr->type ) {
                /* Base register + Offset register */
                opr->type = X86_64_OPR_ADDR;
                opr->u.addr.flags = X86_64_ADDR_BASE | X86_64_ADDR_OFFSET
                    | X86_64_ADDR_SCALE;
                opr->u.addr.base = lopr->u.reg;
                opr->u.addr.offset = ropr->u.reg;
                opr->u.addr.scale = 1;
                opr->u.addr.saddrsize = 0;
                opr->sopsize = 0;
            } else if ( X86_64_OPR_ADDR == lopr->type ) {
                if ( X86_64_OPR_REG == ropr->type ) {
                    /* Base register */
                    if ( X86_64_ADDR_BASE & lopr->u.addr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.addr.flags = X86_64_ADDR_BASE | lopr->u.addr.flags;
                    opr->u.addr.base = ropr->u.reg;
                    opr->u.addr.offset = lopr->u.addr.offset;
                    opr->u.addr.scale = lopr->u.addr.scale;
                    (void)memcpy(&opr->u.addr.disp, &lopr->u.addr.disp,
                                 sizeof(x86_64_imm_t));
                    opr->u.addr.saddrsize = 0;
                    opr->sopsize = 0;
                } else if ( X86_64_OPR_IMM == ropr->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & lopr->u.addr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.addr.flags = X86_64_ADDR_DISP | lopr->u.addr.flags;
                    (void)memcpy(&opr->u.addr.disp, &ropr->u.imm,
                                 sizeof(x86_64_imm_t));
                    opr->u.addr.base = lopr->u.addr.base;
                    opr->u.addr.offset = lopr->u.addr.offset;
                    opr->u.addr.scale = lopr->u.addr.scale;
                    opr->u.addr.saddrsize = 0;
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
                    if ( X86_64_ADDR_BASE & ropr->u.addr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.addr.flags = X86_64_ADDR_BASE | ropr->u.addr.flags;
                    opr->u.addr.base = lopr->u.reg;
                    opr->u.addr.saddrsize = 0;
                    opr->sopsize = 0;
                } else if ( X86_64_OPR_IMM == lopr->type ) {
                    /* Displacement */
                    if ( X86_64_ADDR_DISP & ropr->u.addr.flags ) {
                        /* Invalid syntax */
                        free(opr);
                        free(lopr);
                        free(ropr);
                        return NULL;
                    }
                    opr->type = X86_64_OPR_ADDR;
                    opr->u.addr.flags = X86_64_ADDR_DISP | ropr->u.addr.flags;
                    (void)memcpy(&opr->u.addr.disp, &lopr->u.imm,
                                 sizeof(x86_64_imm_t));
                    opr->u.addr.saddrsize = 0;
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
                    opr->u.addr.flags = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    opr->u.addr.offset = ropr->u.reg;
                    opr->u.addr.scale = lopr->u.imm.u.fixed;
                    opr->u.addr.saddrsize = 0;
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
                    opr->u.addr.flags = X86_64_ADDR_OFFSET | X86_64_ADDR_SCALE;
                    opr->u.addr.offset = lopr->u.reg;
                    opr->u.addr.scale = ropr->u.imm.u.fixed;
                    opr->u.addr.saddrsize = 0;
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
        opr->u.addr.flags = X86_64_ADDR_BASE;
        opr->u.addr.base = reg;
        opr->u.addr.saddrsize = opr->sopsize;
        opr->sopsize = 0;
    } else if ( X86_64_OPR_IMM == opr->type ) {
        (void)memcpy(&imm, &opr->u.imm, sizeof(x86_64_imm_t));
        opr->type = X86_64_OPR_ADDR;
        opr->u.addr.flags = X86_64_ADDR_DISP;
        (void)memcpy(&opr->u.addr.disp, &imm, sizeof(x86_64_imm_t));
        opr->u.addr.saddrsize = opr->sopsize;
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
        if ( X86_64_ADDR_BASE & opr->u.addr.flags ) {
            sz = regsize(opr->u.addr.base);
            if ( 0 != opr->u.addr.saddrsize
                 && sz != opr->u.addr.saddrsize ) {
                /* Invalid operand size */
                free(opr);
                return NULL;
            }
            opr->u.addr.saddrsize = sz;
        }
        if ( X86_64_ADDR_OFFSET & opr->u.addr.flags ) {
            sz = regsize(opr->u.addr.offset);
            if ( 0 != opr->u.addr.saddrsize
                 && sz != opr->u.addr.saddrsize ) {
                /* Invalid operand size */
                free(opr);
                return NULL;
            }
            opr->u.addr.saddrsize = sz;
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

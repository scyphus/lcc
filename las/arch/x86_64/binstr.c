/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "../../las.h"
#include "reg.h"
#include "eval.h"
#include "binstr.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define REX_NE          -2      /* Not encodable */
#define REX_NONE        -1
#define REX_FALSE       0
#define REX_TRUE        1

/*     return (1<<6) | (w<<3) | (r<<2) | (x<<1) | b; */
#define REX             (1<<6)
#define REX_W           (1<<3)
#define REX_R           (1<<2)
#define REX_X           (1<<1)
#define REX_B           (1)


static int
_encode_rm(const x86_64_opr_t *, const x86_64_opr_t *, x86_64_enop_t *);
static int
_encode_mr(const x86_64_opr_t *, const x86_64_opr_t *, x86_64_enop_t *);
static int
_encode_oi(const x86_64_opr_t *, const x86_64_opr_t *, size_t,
           x86_64_enop_t *);
static int
_encode_mi(const x86_64_opr_t *, const x86_64_opr_t *, int, size_t,
           x86_64_enop_t *);


/*
 * Is an immediate value? (imm8, imm16, imm32, or imm64)
 */
static __inline__ int
_is_imm(x86_64_opr_t *opr, int sz)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == opr->sopsize ) {
                /* Operand size is not specified */
                return 1;
            } else if ( sz == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( 0 == opr->sopsize )  {
                /* Operand size is not specified */
                return 1;
            } else if ( sz == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm8(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == opr->sopsize ) {
                /* Operand size is not specified */
                if ( opr->u.imm.u.fixed >= -128
                     && opr->u.imm.u.fixed <= 127 ) {
                    /* In the range of imm8 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE8 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE8 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm16(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == opr->sopsize ) {
                /* Operand size is not specified */
                if ( opr->u.imm.u.fixed >= -32768
                     && opr->u.imm.u.fixed <= 32767 ) {
                    /* In the range of imm16 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE16 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE16 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm32(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == opr->sopsize ) {
                /* Operand size is not specified */
                if ( opr->u.imm.u.fixed >= -2147483648
                     && opr->u.imm.u.fixed <= 2147483647 ) {
                    /* In the range of imm32 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE32 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE32 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else if ( 0 == opr->sopsize ) {
                return 1;
            } else {
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm64(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == opr->sopsize ) {
                /* Operand size is not specified */
                return 1;
            } else if ( SIZE64 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE64 == opr->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_3(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        switch ( opr->u.imm.type ) {
        case X86_64_IMM_FIXED:
            if ( 3 == opr->u.imm.u.fixed ) {
                return 1;
            }
            break;
        default:
            ;
        }
    }
    return 0;
}


/*
 * Is the estimated value an X-bit relative address?
 */
#if 0
static __inline__ int
_is_rel8(x86_64_opr_t *opr, off_t pos)
{
    int64_t imin;
    int64_t imax;

    if ( X86_64_OPR_IMM == opr->type ) {
        /* Fixed value */
        if ( 0 == opr->sopsize ) {
            //imin = opr->u.imm.u.est.min - pos * X86_64_INSTR_SIZE_MAX;
            //imax = opr->u.imm.u.est.max - pos * X86_64_INSTR_SIZE_MIN;
            /* Operand size is not specified */
            if ( imin >= -128 && imax <= 127 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE8 == opr->sopsize ) {
            /* Operand size is specified */
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm16(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type
         && X86_64_IMM_FIXED == opr->u.imm.type ) {
        /* Fixed value */
        if ( 0 == opr->sopsize ) {
            /* Operand size is not specified */
            if ( opr->u.imm.u.fixed >= -32768
                 && opr->u.imm.u.fixed <= 32767 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE16 == opr->sopsize ) {
            /* Operand size is specified */
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm32(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type
         && X86_64_IMM_FIXED == opr->u.imm.type ) {
        /* Fixed value */
        if ( 0 == opr->sopsize ) {
            /* Operand size is not specified */
            if ( opr->u.imm.u.fixed >= -2147483648
                 && opr->u.imm.u.fixed <= 2147483647 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE32 == opr->sopsize ) {
            /* Operand size is specified */
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm64(x86_64_opr_t *opr)
{
    if ( X86_64_OPR_IMM == opr->type ) {
        if ( 0 == opr->sopsize ) {
            return 1;
        } else if ( SIZE64 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
#endif


/*
 * Does the estimated value equal the specified register?
 */
static __inline__ int
_eq_reg(const x86_64_opr_t *opr, x86_64_reg_t reg)
{
    if ( X86_64_OPR_REG == opr->type && reg == opr->u.reg ) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Is the register with the specified size?
 */
static __inline__ int
_is_r16(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE16 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_r32(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE32 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_r64(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE64 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

/*
 * Is the value type memory with the specified size?
 */
static __inline__ int
_is_m_unspec(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( 0 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_m8(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE8 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_m16(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE16 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_m32(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE32 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_m64(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE64 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

/*
 * Is the estimated value register or memory operand with the specified size?
 */
static __inline__ int
_is_rm8(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type || X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE8 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm16(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type || X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE16 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm32(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type || X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE32 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm64(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type || X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE64 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}





static __inline__ int
_is_reg(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg8(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE8 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg16(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE16 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg32(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE32 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg64(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_REG == opr->type ) {
        if ( SIZE64 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_addr8(const x86_64_opr_t *opr)
{
    if ( X86_64_OPR_ADDR == opr->type ) {
        if ( SIZE8 == opr->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}



static __inline__ int
_is_rm8_r8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE8 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE8 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE32 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE8 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE64 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE8 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE16 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE16 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE32 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE16 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm32_r32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE32 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE32 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm64_r64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr2->type && SIZE64 == opr2->sopsize
         && (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && (0 == opr1->sopsize || SIZE64 == opr1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_r8_rm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm8_r8(opr2, opr1);
}
static __inline__ int
_is_r16_rm16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm16_r16(opr2, opr1);
}
static __inline__ int
_is_r32_rm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm8_r32(opr2, opr1);
}
static __inline__ int
_is_r32_rm16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm16_r32(opr2, opr1);
}
static __inline__ int
_is_r32_rm32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm32_r32(opr2, opr1);
}
static __inline__ int
_is_r64_rm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm8_r64(opr2, opr1);
}
static __inline__ int
_is_r64_rm64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    return _is_rm64_r64(opr2, opr1);
}
static __inline__ int
_is_r8_imm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE8 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize ) {
            if ( opr2->u.imm.u.fixed >= -128 && opr2->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_imm16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE16 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize ) {
            if ( opr2->u.imm.u.fixed >= -32768
                 && opr2->u.imm.u.fixed <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_imm32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE32 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize ) {
            if ( opr2->u.imm.u.fixed >= -2147483648
                 && opr2->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_imm64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE64 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize ) {
            return 1;
        } else if ( SIZE64 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r8_rela8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE8 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize || SIZE8 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rela16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE16 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize || SIZE16 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rela32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE32 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize || SIZE32 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rela64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( X86_64_OPR_REG == opr1->type && SIZE64 == opr1->sopsize
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        if ( 0 == opr2->sopsize || SIZE64 == opr2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_rm8_imm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( SIZE8 == opr1->sopsize ) {
            if ( 0 == opr2->sopsize ) {
                if ( opr2->u.imm.u.fixed >= -128
                     && opr2->u.imm.u.fixed <= 127 ) {
                    return 1;
                }
            } else if ( SIZE8 == opr2->sopsize ) {
                return 1;
            }
        } else if ( 0 == opr1->sopsize ) {
            if ( SIZE8 == opr2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm16_imm16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( SIZE16 == opr1->sopsize ) {
            if ( 0 == opr2->sopsize ) {
                if ( opr2->u.imm.u.fixed >= -32768
                     && opr2->u.imm.u.fixed <= 32767 ) {
                    return 1;
                }
            } else if ( SIZE16 == opr2->sopsize ) {
                return 1;
            }
        } else if ( 0 == opr1->sopsize ) {
            if ( SIZE16 == opr2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm32_imm32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( SIZE32 == opr1->sopsize ) {
            if ( 0 == opr2->sopsize ) {
                if ( opr2->u.imm.u.fixed >= -2147483648
                     && opr2->u.imm.u.fixed <= 2147483647 ) {
                    return 1;
                }
            } else if ( SIZE32 == opr2->sopsize ) {
                return 1;
            }
        } else if ( 0 == opr1->sopsize ) {
            if ( SIZE32 == opr2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm64_imm64(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2)
{
    /* Check the first and second operands */
    if ( (X86_64_OPR_REG == opr1->type || X86_64_OPR_ADDR == opr1->type)
         && X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        if ( SIZE64 == opr1->sopsize ) {
            if ( 0 == opr2->sopsize ) {
                return 1;
            } else if ( SIZE64 == opr2->sopsize ) {
                return 1;
            }
        } else if ( 0 == opr1->sopsize ) {
            if ( SIZE64 == opr2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
/* RMI */
static __inline__ int
_is_r16_rm16_imm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                  const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE16 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE16 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -128 && opr3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                  const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE32 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE32 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -128 && opr3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm8(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                  const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE64 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE64 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -128 && opr3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rm16_imm16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                   const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE16 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE16 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -32768
                 && opr3->u.imm.u.fixed <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rm16_rela16(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                   const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_EXPR == opr3->u.imm.type ) {
        if ( SIZE16 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE16 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize || SIZE16 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                   const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE32 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE32 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -2147483648
                 && opr3->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_rela32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                   const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_EXPR == opr3->u.imm.type ) {
        if ( SIZE32 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE32 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize || SIZE32 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                   const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        if ( SIZE64 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE64 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize ) {
            if ( opr3->u.imm.u.fixed >= -2147483648
                 && opr3->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_rela32(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
                    const x86_64_opr_t *opr3)
{
    /* Check the type of all operands */
    if ( X86_64_OPR_REG == opr1->type
         && (X86_64_OPR_REG == opr2->type || X86_64_OPR_ADDR == opr2->type)
         && X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_EXPR == opr3->u.imm.type ) {
        if ( SIZE64 != opr1->sopsize ) {
            return 0;
        }
        if ( 0 != opr2->sopsize && SIZE64 != opr2->sopsize ) {
            return 0;
        }
        if ( 0 == opr3->sopsize || SIZE32 == opr3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}



/*
 * Resolve register code (and REX.B)
 * Vol. 2A 3-2
 */
static int
_reg_code(x86_64_reg_t reg, int *code, int *rex)
{
    switch ( reg ) {
    case REG_AL:
    case REG_AX:
    case REG_EAX:
    case REG_RAX:
        *code = 0;
        *rex = REX_NONE;
        break;
    case REG_CL:
    case REG_CX:
    case REG_ECX:
    case REG_RCX:
        *code = 1;
        *rex = REX_NONE;
        break;
    case REG_DL:
    case REG_DX:
    case REG_EDX:
    case REG_RDX:
        *code = 2;
        *rex = REX_NONE;
        break;
    case REG_BL:
    case REG_BX:
    case REG_EBX:
    case REG_RBX:
        *code = 3;
        *rex = REX_NONE;
        break;
    case REG_AH:
        *code = 4;
        *rex = REX_NE;
        break;
    case REG_SP:
    case REG_ESP:
    case REG_RSP:
        *code = 4;
        *rex = REX_NONE;
        break;
    case REG_SPL:
        *code = 4;
        *rex = REX_FALSE;
        break;
    case REG_CH:
        *code = 5;
        *rex = REX_NE;
        break;
    case REG_BP:
    case REG_EBP:
    case REG_RBP:
        *code = 5;
        *rex = REX_NONE;
        break;
    case REG_BPL:
        *code = 5;
        *rex = REX_FALSE;
        break;
    case REG_DH:
        *code = 6;
        *rex = REX_NE;
        break;
    case REG_SI:
    case REG_ESI:
    case REG_RSI:
        *code = 6;
        *rex = REX_NONE;
        break;
    case REG_SIL:
        *code = 6;
        *rex = REX_FALSE;
        break;
    case REG_BH:
        *code = 7;
        *rex = REX_NE;
        break;
    case REG_DI:
    case REG_EDI:
    case REG_RDI:
        *code = 7;
        *rex = REX_NONE;
        break;
    case REG_DIL:
        *code = 7;
        *rex = REX_FALSE;
        break;
    case REG_R8L:
    case REG_R8W:
    case REG_R8D:
    case REG_R8:
        *code = 0;
        *rex = REX_TRUE;
        break;
    case REG_R9L:
    case REG_R9W:
    case REG_R9D:
    case REG_R9:
        *code = 1;
        *rex = REX_TRUE;
        break;
    case REG_R10L:
    case REG_R10W:
    case REG_R10D:
    case REG_R10:
        *code = 2;
        *rex = REX_TRUE;
        break;
    case REG_R11L:
    case REG_R11W:
    case REG_R11D:
    case REG_R11:
        *code = 3;
        *rex = REX_TRUE;
        break;
    case REG_R12L:
    case REG_R12W:
    case REG_R12D:
    case REG_R12:
        *code = 4;
        *rex = REX_TRUE;
        break;
    case REG_R13L:
    case REG_R13W:
    case REG_R13D:
    case REG_R13:
        *code = 5;
        *rex = REX_TRUE;
        break;
    case REG_R14L:
    case REG_R14W:
    case REG_R14D:
    case REG_R14:
        *code = 6;
        *rex = REX_TRUE;
        break;
    case REG_R15L:
    case REG_R15W:
    case REG_R15D:
    case REG_R15:
        *code = 7;
        *rex = REX_TRUE;
        break;
    default:
        return -1;
    }

    return 0;
}






/*
 * Encode REX prefix
 * REX prefix = [0100WRXB]
 */
int
_rex(int w, int r, int x, int b)
{
    int rex;

    if ( (REX_NONE == w || REX_NE == w) && (REX_NONE == r || REX_NE == r)
         && (REX_NONE == x || REX_NE == x) && (REX_NONE == b || REX_NE == b) ) {
        return 0;
    }

    if ( REX_NE == w || REX_NE == r || REX_NE == x || REX_NE == b ) {
        /* REX is not encodable but has REX, i.e., must raise an error */
        return -1;
    }

    rex = REX;
    if ( REX_TRUE == w ) {
        rex |= REX_W;
    }
    if ( REX_TRUE == r ) {
        rex |= REX_R;
    }
    if ( REX_TRUE == x ) {
        rex |= REX_X;
    }
    if ( REX_TRUE == b ) {
        rex |= REX_B;
    }

    return rex;
}

ssize_t
_resolve_operand_size1(const x86_64_opr_t *opr)
{
    size_t s1;
    size_t s2;

    /* Specified operand size */
    s1 = opr->sopsize;

    /* Estimated operand size */
    switch ( opr->type ) {
    case X86_64_OPR_REG:
        s2 = regsize(opr->u.reg);
        break;
    default:
        s2 = 0;
    }

    if ( 0 == s1 ) {
        s1 = s2;
    } else if ( 0 != s2 && s1 != s2 ){
        return -1;
    }

    return s1;
}
ssize_t
_resolve_address_size1(const x86_64_opr_t *opr)
{
    size_t s1;
    size_t s2;
    size_t s3;

    if ( X86_64_OPR_ADDR != opr->type ) {
        return 0;
    }

    /* Specified address size */
    s1 = opr->u.eaddr.saddrsize;

    /* Estimated operand size */
    if ( X86_64_ADDR_BASE & opr->u.eaddr.flags ) {
        s2 = regsize(opr->u.eaddr.base);
    } else {
        s2 = 0;
    }
    if ( X86_64_ADDR_OFFSET & opr->u.eaddr.flags ) {
        s3 = regsize(opr->u.eaddr.offset);
    } else {
        s3 = 0;
    }

    if ( 0 == s1 ) {
        s1 = s2;
    } else if ( 0 != s2 && s1 != s2 ){
        return -1;
    }
    if ( 0 == s1 ) {
        s1 = s3;
    } else if ( 0 != s3 && s1 != s3 ){
        return -1;
    }

    return s1;
}


/*
 * Vol. 2A 2-5
 * Mod(2) | Reg(3) | R/M(3)
 */

/*
 * Encode ModR/M
 */
static int
_encode_modrm(int reg, int mod, int rm)
{
    return (mod << 6) | (reg << 3) | rm;
}

/*
 * Encode SIB
 */
static int
_encode_sib(int base, int idx, int ss)
{
    return (ss << 6) | (idx << 3) | base;
}

/*
 * Encode scaled index
 */
static int
_encode_scaled_index(const x86_64_opr_t *opr, int *idx, int *rexx, int *ss)
{
    int ret;

    assert( X86_64_OPR_ADDR == opr->type );

    /* Encode scaled index */
    if ( X86_64_ADDR_OFFSET & opr->u.eaddr.flags ) {
        /* Resolve the register code of the index register */
        ret = _reg_code(opr->u.eaddr.offset, idx, rexx);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }
        if ( 4 == *idx && (REX_NONE == *rexx || REX_FALSE == *rexx) ) {
            /* Invalid index register */
            return -1;
        }

        /* Check scale multiplier */
        if ( X86_64_ADDR_SCALE & opr->u.eaddr.flags ) {
            switch ( opr->u.eaddr.scale ) {
            case 1:
                *ss = 0;
                break;
            case 2:
                *ss = 1;
                break;
            case 4:
                *ss = 2;
                break;
            case 8:
                *ss = 3;
                break;
            default:
                /* Invalid scale multiplier */
                return -1;
            }
        } else {
            *ss = 0;
        }
    } else {
        /* No index register */
        *idx = 4;
        *ss = 0;
        *rexx = REX_NONE;
    }

    return 0;
}

/*
 * Encode displacement
 */
static int
_encode_disp(const x86_64_opr_t *opr, int64_t *disp, size_t *dispsz)
{
    assert( X86_64_OPR_ADDR == opr->type );

    if ( opr->u.eaddr.flags & X86_64_ADDR_DISP ) {
        /* Displacement is specified */
        if ( X86_64_IMM_FIXED == opr->u.eaddr.disp.type ) {
            /* Get the value of displacement */
            *disp = opr->u.eaddr.disp.u.fixed;
            if ( 0 == *disp ) {
                *dispsz = 0;
            } else if ( *disp >= -128 && *disp <= 127 ) {
                *dispsz = SIZE8;
            } else {
                *dispsz = SIZE32;
            }
        } else {
            /* Relocatable displacement */
            *dispsz = SIZE32;
            *disp = 0;
        }
    } else {
        /* No displacement */
        *dispsz = 0;
        *disp = 0;
    }

    return 0;
}


/*
 * Encode the second operand with the addr type that specifies a base register
 */
static int
_encode_rm_addr_with_base(int reg, int rexr, const x86_64_opr_t *opr,
                          x86_64_enop_t *enop)
{
    int ret;
    int mod;
    int rm;
    int modrm;
    int base;
    int idx;
    int ss;
    int sib;
    int rexx;
    int rexb;
    size_t dispsz;
    int64_t disp;

    assert( X86_64_OPR_ADDR == opr->type );
    assert( X86_64_ADDR_BASE & opr->u.eaddr.flags );

    rexx = REX_NONE;
    rexb = REX_NONE;

    /* Is the base register EIP/RIP? */
    if ( REG_EIP == opr->u.eaddr.base || REG_RIP == opr->u.eaddr.base ) {
        /* The base register is EIP/RIP */
        if ( (X86_64_ADDR_OFFSET & opr->u.eaddr.flags)
             || (X86_64_ADDR_SCALE & opr->u.eaddr.flags) ) {
            /* Must not have scaled index */
            return -1;
        }
        mod = 0;
        rm = 5;

        /* Encode ModR/M */
        modrm = _encode_modrm(reg, mod, rm);
        sib = -1;

        /* Encode displacement */
        ret = _encode_disp(opr, &disp, &dispsz);
        /* Extend the size of displacement to 4 bytes */
        dispsz = 4;

        /* Opcode register */
        enop->opreg = -1;
        /* REX */
        enop->rex.r = rexr;
        enop->rex.x = REX_NONE;
        enop->rex.b = REX_NONE;
        /* ModR/M */
        enop->modrm = modrm;
        /* SIB */
        enop->sib = sib;
        /* Displacement */
        enop->disp.sz = dispsz;
        enop->disp.val = disp;
        enop->disp.eval = opr;
        /* FIXME */
        enop->disp.expr = NULL;
        /* No immediate value */
        enop->imm.sz = 0;
        enop->imm.val = 0;
        enop->imm.eval = NULL;
        enop->imm.expr = NULL;
        /* No relative value */
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;
        /* No ptr value */
        enop->ptr0.sz = 0;
        enop->ptr0.val = 0;
        enop->ptr0.expr = NULL;
        enop->ptr1.sz = 0;
        enop->ptr1.val = 0;
        enop->ptr1.expr = NULL;

        return 0;
    } else {
        /* The base register is not EIP/RIP and needs to specify SIB */

        /* Resolve the register code of the base register */
        ret = _reg_code(opr->u.eaddr.base, &base, &rexb);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }

        /* Encode displacement */
        ret = _encode_disp(opr, &disp, &dispsz);
        if ( ret < 0 ) {
            return -1;
        }

        if ( X86_64_ADDR_OFFSET & opr->u.eaddr.flags ) {
            /* Requires SIB */
            ret = _encode_scaled_index(opr, &idx, &rexx, &ss);
            if ( ret < 0 ) {
                return -1;
            }

            /* Check the base register */
            if ( 5 == base && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                /* Special encoding is required if it's EBP/RBP */
                if ( 0 == dispsz || 1 == dispsz ) {
                    mod = 1;
                    rm = 4;
                    dispsz = 1;
                } else if ( 4 == dispsz ) {
                    mod = 2;
                    rm = 4;
                } else {
                    return -1;
                }
            } else {
                if ( 0 == dispsz ) {
                    mod = 0;
                    rm = 4;
                } else if ( 1 == dispsz ) {
                    mod = 1;
                    rm = 4;
                } else if ( 4 == dispsz ) {
                    mod = 2;
                    rm = 4;
                } else {
                    return -1;
                }
            }

            /* Encode ModR/M */
            modrm = _encode_modrm(reg, mod, rm);
            sib = _encode_sib(base, idx, ss);
        } else {
            /* Scaled index is not specified */
            if ( 0 == dispsz ) {
                if ( 5 == base && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                    /* Set dispsz = 1 */
                    mod = 1;
                    rm = 5;
                    dispsz = 1;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = -1;
                } else if ( 4 == base
                            && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                    /* Go to SIB */
                    mod = 0;
                    rm = 4;
                    idx = 4;
                    ss = 0;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = _encode_sib(base, idx, ss);
                } else {
                    /* ModR/M */
                    mod = 0;
                    rm = base;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = -1;
                }
            } else if ( 1 == dispsz ) {
                mod = 1;
                if ( 4 == base && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                    /* Go to SIB */
                    rm = 4;
                    idx = 4;
                    ss = 0;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = _encode_sib(base, idx, ss);
                } else {
                    /* ModR/M */
                    rm = base;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = -1;
                }
            } else if ( 4 == dispsz ) {
                mod = 2;
                if ( 4 == base && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                    /* Go to SIB */
                    rm = 4;
                    idx = 4;
                    ss = 0;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = _encode_sib(base, idx, ss);
                } else {
                    /* ModR/M */
                    rm = base;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = -1;
                }
            } else {
                return -1;
            }
        }

        /* Opcode register */
        enop->opreg = -1;
        /* REX */
        enop->rex.r = rexr;
        enop->rex.x = rexx;
        enop->rex.b = rexb;
        /* ModR/M */
        enop->modrm = modrm;
        /* SIB */
        enop->sib = sib;
        /* Displacement */
        enop->disp.sz = dispsz;
        enop->disp.val = disp;
        enop->disp.eval = opr;
        /* FIXME */
        enop->disp.expr = NULL;
        /* No immediate value */
        enop->imm.sz = 0;
        enop->imm.val = 0;
        enop->imm.eval = NULL;
        enop->imm.expr = NULL;
        /* No relative value */
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;
        /* No ptr value */
        enop->ptr0.sz = 0;
        enop->ptr0.val = 0;
        enop->ptr0.expr = NULL;
        enop->ptr1.sz = 0;
        enop->ptr1.val = 0;
        enop->ptr1.expr = NULL;

        return 0;
    }
}

/*
 * Encode the second operand with the addr type for the w/o base register case
 */
static int
_encode_rm_addr_without_base(int reg, int rexr, const x86_64_opr_t *opr,
                             x86_64_enop_t *enop)
{
    int ret;
    int mod;
    int rm;
    int base;
    int idx;
    int ss;
    int rexx;
    size_t dispsz;
    int64_t disp;

    assert( X86_64_OPR_ADDR == opr->type );
    assert( !(X86_64_ADDR_BASE & opr->u.eaddr.flags) );

    /* Mod and R/M must be 0 and 4, respectively, for the case w/o base
       register */
    mod = 0;
    rm = 4;
    /* The base register 5 where Mod is 5 means "w/o base register" */
    base = 5;

    ret = _encode_scaled_index(opr, &idx, &rexx, &ss);
    if ( ret < 0 ) {
        return -1;
    }

    /* Encode displacement */
    ret = _encode_disp(opr, &disp, &dispsz);
    if ( ret < 0 ) {
        return -1;
    }
    /* Thie only supports disp32 */
    dispsz = 4;

    /* Opcode register */
    enop->opreg = -1;
    /* REX */
    enop->rex.r = rexr;
    enop->rex.x = rexx;
    enop->rex.b = REX_NONE;
    /* ModR/M */
    enop->modrm = _encode_modrm(reg, mod, rm);
    /* SIB */
    enop->sib = _encode_sib(base, idx, ss);
    /* Set displacement */
    enop->disp.sz = dispsz;
    enop->disp.val = disp;
    enop->disp.eval = opr;
    /* FIXME */
    enop->disp.expr = NULL;
    /* No immediate value */
    enop->imm.sz = 0;
    enop->imm.val = 0;
    enop->imm.eval = NULL;
    enop->imm.expr = NULL;
    /* No relative value */
    enop->rel.sz = 0;
    enop->rel.val = 0;
    enop->rel.expr = NULL;
    /* No ptr value */
    enop->ptr0.sz = 0;
    enop->ptr0.val = 0;
    enop->ptr0.expr = NULL;
    enop->ptr1.sz = 0;
    enop->ptr1.val = 0;
    enop->ptr1.expr = NULL;

    return 0;
}

/*
 * Encode the second R/M of RM operands
 */
static int
_encode_rm_addr(int reg, int rexr, const x86_64_opr_t *opr, x86_64_enop_t *enop)
{
    int ret;

    assert( X86_64_OPR_ADDR == opr->type );

    if ( X86_64_ADDR_BASE & opr->u.eaddr.flags ) {
        /* With base register */
        ret = _encode_rm_addr_with_base(reg, rexr, opr, enop);
    } else {
        /* Without base register */
        ret = _encode_rm_addr_without_base(reg, rexr, opr, enop);
    }

    return 0;
}

/*
 * Encode the second register of RM operands
 */
static int
_encode_rm_reg(int reg, int rexr, const x86_64_opr_t *opr, x86_64_enop_t *enop)
{
    int ret;
    int mod;
    int rm;
    int rexb;
    int modrm;

    assert( X86_64_OPR_REG == opr->type );

    /* Mod = 3 where  R/M is register */
    mod = 3;

    /* Resolve the register code of the second operand */
    ret = _reg_code(opr->u.reg, &rm, &rexb);
    if ( ret < 0 ) {
        /* FIXME!!! */
        /* Cannot resolve the register code */
        return -1;
    }
    /* Encode ModR/M */
    modrm = _encode_modrm(reg, mod, rm);

    /* Opcode register */
    enop->opreg = -1;
    /* REX */
    enop->rex.r = rexr;
    enop->rex.x = REX_NONE;
    enop->rex.b = rexb;
    /* ModR/M */
    enop->modrm = modrm;
    /* No SIB */
    enop->sib = -1;
    /* No displacement */
    enop->disp.sz = 0;
    enop->disp.val = 0;
    enop->disp.eval = NULL;
    enop->disp.expr = NULL;
    /* No immediate value */
    enop->imm.sz = 0;
    enop->imm.val = 0;
    enop->imm.eval = NULL;
    enop->imm.expr = NULL;
    /* No relative value */
    enop->rel.sz = 0;
    enop->rel.val = 0;
    enop->rel.expr = NULL;
    /* No ptr value */
    enop->ptr0.sz = 0;
    enop->ptr0.val = 0;
    enop->ptr0.expr = NULL;
    enop->ptr1.sz = 0;
    enop->ptr1.val = 0;
    enop->ptr1.expr = NULL;

    return 0;
}

/*
 * Encode operands whose Op/En is RM
 */
static int
_encode_rm(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_OPR_REG != opr1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(opr1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_OPR_REG == opr2->type ) {
        ret = _encode_rm_reg(reg, rexr, opr2, enop);
        if ( ret < 0 ) {
            return -1;
        }

        return 0;
    } else if ( X86_64_OPR_ADDR == opr2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, opr2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

/*
 * Encode operands whose Op/En is MR
 */
static int
_encode_mr(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
           x86_64_enop_t *enop)
{
    return _encode_rm(opr2, opr1, enop);
}

/*
 * Encode operands whose Op/En is RMI
 */
static int
_encode_rmi(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2,
            const x86_64_opr_t *opr3, size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_OPR_REG != opr1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(opr1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_OPR_REG == opr2->type ) {
        ret = _encode_rm_reg(reg, rexr, opr2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_OPR_ADDR == opr2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, opr2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    /* Check the third operand */
    if ( X86_64_OPR_IMM == opr3->type
         && X86_64_IMM_FIXED == opr3->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = opr3->u.imm.u.fixed;
        enop->imm.eval = NULL;
    } else if ( X86_64_OPR_IMM == opr3->type
                && X86_64_IMM_EXPR == opr3->u.imm.type ) {
        /* Update relocatable value */
        enop->imm.sz = immsz;
        enop->imm.val = 0;
        enop->imm.eval = opr3;
    } else {
        return -1;
    }

    return 0;
}

#if 0
static int
_encode_fd(const x86_64_val_t *val1, const x86_64_val_t *val2,
           x86_64_enop_t *enop)
{
    /* To be implemented */
    return -1;
}
static int
_encode_td(const x86_64_val_t *val1, const x86_64_val_t *val2,
           x86_64_enop_t *enop)
{
    /* To be implemented */
    return -1;
}
#endif
static int
_encode_oi(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2, size_t immsz,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_OPR_REG != opr1->type ) {
        return -1;
    }
    ret = _reg_code(opr1->u.reg, &reg, &rexb);
    if ( ret < 0 ) {
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        /* Immediate value */
        enop->opreg = reg;
        enop->rex.r = REX_NONE;
        enop->rex.x = REX_NONE;
        enop->rex.b = rexb;
        enop->modrm = -1;
        enop->sib = -1;
        enop->disp.sz = 0;
        enop->disp.val = 0;
        enop->disp.eval = NULL;
        enop->disp.expr = NULL;
        enop->imm.sz = immsz;
        enop->imm.val = opr2->u.imm.u.fixed;
        enop->imm.eval = NULL;
        enop->imm.expr = NULL;
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;
        enop->ptr0.sz = 0;
        enop->ptr0.val = 0;
        enop->ptr0.expr = NULL;
        enop->ptr1.sz = 0;
        enop->ptr1.val = 0;
        enop->ptr1.expr = NULL;

        return 0;
    } else if ( X86_64_OPR_IMM == opr2->type
                && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        /* Relocatable address */
        enop->opreg = reg;
        enop->rex.r = REX_NONE;
        enop->rex.x = REX_NONE;
        enop->rex.b = rexb;
        enop->modrm = -1;
        enop->sib = -1;
        enop->disp.sz = 0;
        enop->disp.val = 0;
        enop->disp.eval = NULL;
        enop->disp.expr = NULL;
        enop->imm.sz = immsz;
        enop->imm.val = 0;
        enop->imm.eval = opr2;
        /* FIXME */
        enop->imm.expr = NULL;
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;
        enop->ptr0.sz = 0;
        enop->ptr0.val = 0;
        enop->ptr0.expr = NULL;
        enop->ptr1.sz = 0;
        enop->ptr1.val = 0;
        enop->ptr1.expr = NULL;

        return 0;
    } else {
        return -1;
    }
}

/*
 * Encode the MI type Op/En
 */
static int
_encode_mi(const x86_64_opr_t *opr1, const x86_64_opr_t *opr2, int reg,
           size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_OPR_REG == opr1->type ) {
        ret = _encode_rm_reg(reg, rexr, opr1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_OPR_ADDR == opr1->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, opr1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }


    /* Check the second operand */
    if ( X86_64_OPR_IMM == opr2->type
         && X86_64_IMM_FIXED == opr2->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = opr2->u.imm.u.fixed;
        enop->imm.expr = NULL;
    } else if ( X86_64_OPR_IMM == opr2->type
                && X86_64_IMM_EXPR == opr2->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = 0;
        enop->imm.expr = opr2->u.imm.u.rexpr;
    } else {
        return -1;
    }

    return 0;
}
static int
_encode_m(const x86_64_opr_t *opr, int reg, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_OPR_REG == opr->type ) {
        ret = _encode_rm_reg(reg, rexr, opr, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_OPR_ADDR == opr->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, opr, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}
static int
_encode_o(const x86_64_opr_t *opr, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_OPR_REG != opr->type ) {
        return -1;
    }
    ret = _reg_code(opr->u.reg, &reg, &rexb);
    if ( ret < 0 ) {
        return -1;
    }

    enop->opreg = reg;
    enop->rex.r = REX_NONE;
    enop->rex.x = REX_NONE;
    enop->rex.b = rexb;
    enop->modrm = -1;
    enop->sib = -1;
    enop->disp.sz = 0;
    enop->disp.val = 0;
    enop->disp.eval = NULL;
    enop->disp.expr = NULL;
    enop->imm.sz = 0;
    enop->imm.val = 0;
    enop->imm.eval = NULL;
    enop->imm.expr = NULL;
    enop->rel.sz = 0;
    enop->rel.val = 0;
    enop->rel.expr = NULL;
    enop->ptr0.sz = 0;
    enop->ptr0.val = 0;
    enop->ptr0.expr = NULL;
    enop->ptr1.sz = 0;
    enop->ptr1.val = 0;
    enop->ptr1.expr = NULL;

    return 0;
}

/*
 * Encode the I type Op/En
 */
static int
_encode_i(const x86_64_opr_t *opr, size_t immsz, x86_64_enop_t *enop)
{
    /* Check the operand type */
    if ( X86_64_OPR_IMM == opr->type ) {
        if ( X86_64_IMM_FIXED == opr->u.imm.type ) {
            enop->opreg = -1;
            enop->rex.r = REX_NONE;
            enop->rex.x = REX_NONE;
            enop->rex.b = REX_NONE;
            enop->modrm = -1;
            enop->sib = -1;
            enop->disp.sz = 0;
            enop->disp.val = 0;
            enop->disp.eval = NULL;
            enop->disp.expr = NULL;
            enop->imm.sz = immsz;
            enop->imm.val = opr->u.imm.u.fixed;
            enop->imm.expr = NULL;
            enop->rel.sz = 0;
            enop->rel.val = 0;
            enop->rel.expr = NULL;
            enop->ptr0.sz = 0;
            enop->ptr0.val = 0;
            enop->ptr0.expr = NULL;
            enop->ptr1.sz = 0;
            enop->ptr1.val = 0;
            enop->ptr1.expr = NULL;

            return 0;
        } else if ( X86_64_IMM_EXPR == opr->u.imm.type ) {
            enop->opreg = -1;
            enop->rex.r = REX_NONE;
            enop->rex.x = REX_NONE;
            enop->rex.b = REX_NONE;
            enop->modrm = -1;
            enop->sib = -1;
            enop->disp.sz = 0;
            enop->disp.val = 0;
            enop->disp.eval = NULL;
            enop->disp.expr = NULL;
            enop->imm.sz = immsz;
            enop->imm.val = 0;
            enop->imm.expr = opr->u.imm.u.rexpr;
            enop->rel.sz = 0;
            enop->rel.val = 0;
            enop->rel.expr = NULL;
            enop->ptr0.sz = 0;
            enop->ptr0.val = 0;
            enop->ptr0.expr = NULL;
            enop->ptr1.sz = 0;
            enop->ptr1.val = 0;
            enop->ptr1.expr = NULL;

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

/*
 * Encode the D type Op/En
 */
static int
_encode_d(const x86_64_opr_t *opr, size_t immsz, x86_64_enop_t *enop)
{
    /* Check the operand type */
    if ( X86_64_OPR_IMM == opr->type ) {
        if ( X86_64_IMM_FIXED == opr->u.imm.type ) {
            enop->opreg = -1;
            enop->rex.r = REX_NONE;
            enop->rex.x = REX_NONE;
            enop->rex.b = REX_NONE;
            enop->modrm = -1;
            enop->sib = -1;
            enop->disp.sz = 0;
            enop->disp.val = 0;
            enop->disp.eval = NULL;
            enop->disp.expr = NULL;
            enop->imm.sz = 0;
            enop->imm.val = 0;
            enop->imm.expr = NULL;
            enop->rel.sz = immsz;
            enop->rel.val = opr->u.imm.u.fixed;
            enop->rel.expr = NULL;
            enop->ptr0.sz = 0;
            enop->ptr0.val = 0;
            enop->ptr0.expr = NULL;
            enop->ptr1.sz = 0;
            enop->ptr1.val = 0;
            enop->ptr1.expr = NULL;

            return 0;
        } else if ( X86_64_IMM_EXPR == opr->u.imm.type ) {
            enop->opreg = -1;
            enop->rex.r = REX_NONE;
            enop->rex.x = REX_NONE;
            enop->rex.b = REX_NONE;
            enop->modrm = -1;
            enop->sib = -1;
            enop->disp.sz = 0;
            enop->disp.val = 0;
            enop->disp.eval = NULL;
            enop->disp.expr = NULL;
            enop->imm.sz = 0;
            enop->imm.val = 0;
            enop->imm.expr = NULL;
            enop->rel.sz = immsz;
            enop->rel.val = 0;
            enop->rel.expr = opr->u.imm.u.rexpr;
            enop->ptr0.sz = 0;
            enop->ptr0.val = 0;
            enop->ptr0.expr = NULL;
            enop->ptr1.sz = 0;
            enop->ptr1.val = 0;
            enop->ptr1.expr = NULL;

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}


/*
 * Evaluate operands
 */
static int
_eval(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    x86_64_opr_t *opr;
    operand_t *op;
    size_t nr;
    size_t i;
    x86_64_opr_vector_t *oprs;

    /* Allocate a vector for evals */
    oprs = mvector_new();
    if ( NULL == oprs ) {
        return -1;
    }

    /* Evaluate operands */
    nr = mvector_size(xstmt->stmt->u.instr->operands);
    for ( i = 0; i < nr; i++ ) {
        /* Obtain operands */
        op = mvector_at(xstmt->stmt->u.instr->operands, i);
        /* Evaluate operands */
        opr = x86_64_estimate_operand(op);
        if ( NULL == opr ) {
            /* FIXME: Free the contents of the vector */
            mvector_delete(oprs);
            /* Error */
            return -EOPERAND;
        }
        if ( NULL == mvector_push_back(oprs, opr) ) {
            free(opr);
            /* FIXME: Free the contents of the vector */
            mvector_delete(oprs);
            /* Error */
            return -EUNKNOWN;
        }
    }

    /* Set evals */
    xstmt->oprs = oprs;

    return 1;
}

























/*
 * Encode instruction from encoded operands
 */
static int
_encode_instr(x86_64_instr_t *instr, const x86_64_enop_t *enop,
              x86_64_target_t tgt, int prefix, ssize_t opsize, size_t addrsize)
{
    int rex;
    int p66 = 0;
    int p67 = 0;
    int rexw = REX_NONE;

    switch ( tgt ) {
    case X86_64_O16:
        /* o16 or D flag=0 */
        switch ( opsize ) {
        case SIZE16:
            p66 = 0;
            break;
        case SIZE32:
            p66 = 1;
            break;
        }
        switch ( addrsize ) {
        case SIZE16:
            p67 = 0;
            break;
        case SIZE32:
            p67 = 1;
            break;
        }
        break;
    case X86_64_O32:
        /* D flag=1 */
        switch ( opsize ) {
        case SIZE16:
            p66 = 1;
            break;
        case SIZE32:
            p66 = 0;
            break;
        }
        switch ( addrsize ) {
        case SIZE16:
            p67 = 1;
            break;
        case SIZE32:
            p67 = 0;
            break;
        }
        break;
    case X86_64_O64:
        switch ( opsize ) {
        case SIZE16:
            p66 = 1;
            rexw = REX_NONE;
            break;
        case SIZE32:
            p66 = 0;
            rexw = REX_NONE;
            break;
        case SIZE64:
            rexw = REX_TRUE;
            break;
        }
        switch ( addrsize ) {
        case SIZE32:
            p67 = 1;
            break;
        case SIZE64:
            p67 = 0;
            break;
        }
        break;
    }

    rex = _rex(rexw, enop->rex.r, enop->rex.x, enop->rex.b);
    if ( rex < 0 ) {
        /* Error */
        return -1;
    }
    if ( 0 == rex ) {
        rex = -1;
    }

    instr->prefix1 = -1;
    instr->prefix2 = -1;
    /* Operand size prefix */
    if ( p66 ) {
        instr->prefix3 = 0x66;
    } else {
        instr->prefix3 = -1;
    }
    /* Address size prefix */
    if ( p67 ) {
        instr->prefix4 = 0x67;
    } else {
        instr->prefix4 = -1;
    }
    /* REX prefix */
    instr->rex = rex;
    instr->modrm = enop->modrm;
    instr->sib = enop->sib;
    instr->disp.sz = enop->disp.sz;
    instr->disp.val = enop->disp.val;
    instr->disp.eval = enop->disp.eval;
    instr->disp.expr = enop->disp.expr;
    instr->imm.sz = enop->imm.sz;
    instr->imm.val = enop->imm.val;
    instr->imm.eval = enop->imm.eval;
    instr->imm.expr = enop->imm.expr;
    instr->rel.sz = enop->rel.sz;
    instr->rel.val = enop->rel.val;
    instr->rel.expr = enop->rel.expr;
    instr->ptr0.sz = enop->ptr0.sz;
    instr->ptr0.val = enop->ptr0.val;
    instr->ptr0.expr = enop->ptr0.expr;
    instr->ptr1.sz = enop->ptr1.sz;
    instr->ptr1.val = enop->ptr1.val;
    instr->ptr1.expr = enop->ptr1.expr;

    /* Opcode prefixes */
    if ( OPCODE_PREFIX_LOCK & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf0;
    }
    if ( OPCODE_PREFIX_CRC32 & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REPNE & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REPNZ & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REP & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_REPE & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_REPZ & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_POPCNT & prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_BRANCH_NOT_TAKEN & prefix ) {
        if ( instr->prefix3 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix3 = 0x2e;
    }
    if ( OPCODE_PREFIX_BRANCH_TAKEN & prefix ) {
        if ( instr->prefix3 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix3 = 0x3e;
    }

    return 0;
}


/*
 * Build instruction for the NP type Op/En
 */
static int
_binstr2_np(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Check the size */
    if ( opsize < 0 ) {
        return -EUNKNOWN;
    }

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Reset the encoded operands */
    enop.opreg = -1;
    enop.rex.r = REX_NONE;
    enop.rex.x = REX_NONE;
    enop.rex.b = REX_NONE;
    enop.modrm = -1;
    enop.sib = -1;
    enop.disp.sz = 0;
    enop.disp.val = 0;
    enop.disp.expr = NULL;
    enop.imm.sz = 0;
    enop.imm.val = 0;
    enop.imm.expr = NULL;
    enop.rel.sz = 0;
    enop.rel.val = 0;
    enop.rel.expr = NULL;
    enop.ptr0.sz = 0;
    enop.ptr0.val = 0;
    enop.ptr0.expr = NULL;
    enop.ptr1.sz = 0;
    enop.ptr1.val = 0;
    enop.ptr1.expr = NULL;

    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }

    /* Set the opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the NP type Op/En with preg value
 */
static int
_binstr2_np_preg(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3,
                 ssize_t opsize, int preg)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Check the size */
    if ( opsize < 0 ) {
        return -EUNKNOWN;
    }

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Reset the encoded operands */
    enop.opreg = -1;
    enop.rex.r = REX_NONE;
    enop.rex.x = REX_NONE;
    enop.rex.b = REX_NONE;
    enop.modrm = _encode_modrm(preg, 3, 0);
    enop.sib = -1;
    enop.disp.sz = 0;
    enop.disp.val = 0;
    enop.disp.expr = NULL;
    enop.imm.sz = 0;
    enop.imm.val = 0;
    enop.imm.expr = NULL;
    enop.rel.sz = 0;
    enop.rel.val = 0;
    enop.rel.expr = NULL;
    enop.ptr0.sz = 0;
    enop.ptr0.val = 0;
    enop.ptr0.expr = NULL;
    enop.ptr1.sz = 0;
    enop.ptr1.val = 0;
    enop.ptr1.expr = NULL;

    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }

    /* Set the opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the I type Op/En
 */
static int
_binstr2_i(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
           const x86_64_opr_t *opr, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode the operand values */
    ret = _encode_i(opr, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }

    /* Set the opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the D type Op/En
 */
static int
_binstr2_d(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
           const x86_64_opr_t *opr, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode the operand values */
    ret = _encode_d(opr, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }

    /* Set the opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the MI type Op/En
 */
static int
_binstr2_mi(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, int preg,
            ssize_t opsize, const x86_64_opr_t *oprm,
            const x86_64_opr_t *opri, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;
    x86_64_instr_t *instr;

    /* Check pseudo register */
    if ( preg < 0 ) {
        return -EUNKNOWN;
    }

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_mi(oprm, opri, preg, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }

    /* Obtain address size */
    addrsize = _resolve_address_size1(oprm);
    if ( addrsize < 0 ) {
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize,
                        addrsize);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }

    /* Set the opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the MR type Op/En
 */
static int
_binstr2_mr(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
            const x86_64_opr_t *oprm, const x86_64_opr_t *oprr)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_mr(oprm, oprr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(oprm);
    if ( addrsize < 0 ) {
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize,
                        addrsize);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the O type Op/En
 */
static int
_binstr2_o(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
           const x86_64_opr_t *oprr)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_o(oprr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* +rw/+rd */
    if ( instr->opcode3 >= 0 ) {
        instr->opcode3 += enop.opreg;
    } else if ( instr->opcode2 >= 0 ) {
        instr->opcode2 += enop.opreg;
    } else {
        instr->opcode1 += enop.opreg;
    }

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the RM type Op/En
 */
static int
_binstr2_rm(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
            const x86_64_opr_t *oprr, const x86_64_opr_t *oprm)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_rm(oprr, oprm, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(oprm);
    if ( addrsize < 0 ) {
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize,
                        addrsize);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the M type Op/En
 */
static int
_binstr2_m(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, int preg,
           ssize_t opsize, const x86_64_opr_t *oprm)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_m(oprm, preg, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the RMI type Op/En
 */
static int
_binstr2_rmi(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
             const x86_64_opr_t *oprr, const x86_64_opr_t *oprm,
             const x86_64_opr_t *opri, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_rmi(oprr, oprm, opri, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(oprm);
    if ( addrsize < 0 ) {
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize,
                        addrsize);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction for the OI type Op/En
 */
static int
_binstr2_oi(x86_64_stmt_t *xstmt, int opc1, int opc2, int opc3, ssize_t opsize,
            const x86_64_opr_t *oprr, const x86_64_opr_t *opri,
            size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    x86_64_instr_t *instr;

    /* Allocaate for the instruction */
    instr = malloc(sizeof(x86_64_instr_t));
    if ( NULL == instr ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_oi(oprr, opri, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Encode instruction */
    ret = _encode_instr(instr, &enop, xstmt->tgt, xstmt->prefix, opsize, 0);
    if ( ret < 0 ) {
        /* Invalid operands */
        free(instr);
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* +rw/+rd */
    if ( instr->opcode3 >= 0 ) {
        instr->opcode3 += enop.opreg;
    } else if ( instr->opcode2 >= 0 ) {
        instr->opcode2 += enop.opreg;
    } else {
        instr->opcode1 += enop.opreg;
    }

    /* Set the instruction and the size */
    if ( NULL == mvector_push_back(xstmt->instrs, instr) ) {
        free(instr);
        return -EUNKNOWN;
    }

    /* Success */
    return 1;
}

/*
 * Build instruction and return a success/error code
 */
int
binstr2(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt, ssize_t opsize,
        int opc1, int opc2, int opc3, x86_64_enc_t enc, int preg)
{
    int ret;
    int stat;

    assert( STMT_INSTR == xstmt->stmt->type );

    /* Evaluate operands first */
    ret = _eval(asmblr, xstmt);
    if ( ret < 0 ) {
        /* Error */
        return ret;
    }

    stat = 0;
    switch ( enc ) {
    case ENC_NP:
        /* Check the number of operands and the format */
        if ( 0 == mvector_size(xstmt->oprs) ) {
            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_3:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_3(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_FS:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_FS) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_GS:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_GS) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AL_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AL)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AL:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_AL) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AX_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AX)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_AX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_EAX_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_EAX)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_EAX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_EAX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_PREG:
        /* Check the number of operands and the format */
        if ( 0 == mvector_size(xstmt->oprs) ) {
            /* Build the instruction */
            stat = _binstr2_np_preg(xstmt, opc1, opc2, opc3, opsize, preg);
        }
        break;

    case ENC_I_IMM8:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE8);
        }
        break;
    case ENC_I_IMM16:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE16);
        }
        break;
    case ENC_I_IMM32:
        /* Check the number of operands and the format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE32);
        }
        break;
    case ENC_I_AL_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AL)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_I_AX_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AX)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_I_AX_IMM16:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AX)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE16);
        }
        break;
    case ENC_I_EAX_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_EAX)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_I_EAX_IMM32:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_EAX)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE32);
        }
        break;
    case ENC_I_RAX_IMM32:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_RAX)
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1), SIZE32);
        }
        break;
    case ENC_I_IMM8_AL:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE8)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_AL) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE8);
        }
        break;
    case ENC_I_IMM8_AX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE8)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_AX) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE8);
        }
        break;
    case ENC_I_IMM8_EAX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE8)
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_EAX) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE8);
        }
        break;

    case ENC_MI_RM8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm8(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_MI_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm16(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE16);
        }
        break;
    case ENC_MI_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm32(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE32);
        }
        break;
    case ENC_MI_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm64(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE32);
        }
        break;
    case ENC_MI_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm16(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_MI_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm32(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_MI_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm64(mvector_at(xstmt->oprs, 0))
             && _is_imm(mvector_at(xstmt->oprs, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;

    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm8_r8(mvector_at(xstmt->oprs, 0),
                           mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm16_r16(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm32_r32(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_rm64_r64(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;

    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r8_rm8(mvector_at(xstmt->oprs, 0),
                           mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r16_rm16(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r32_rm8(mvector_at(xstmt->oprs, 0),
                            mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r32_rm16(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r32_rm32(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r64_rm8(mvector_at(xstmt->oprs, 0),
                            mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r64_rm64(mvector_at(xstmt->oprs, 0),
                             mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1));
        }
        break;

    case ENC_O_R16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_r16(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_r32(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_O_R64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_r64(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_O_AX_R16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_AX)
             && _is_r16(mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_O_EAX_R32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_EAX)
             && _is_r32(mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_O_RAX_R64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _eq_reg(mvector_at(xstmt->oprs, 0), REG_RAX)
             && _is_r64(mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 1));
        }
        break;
    case ENC_O_R16_AX:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r16(mvector_at(xstmt->oprs, 0))
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_AX) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_O_R32_EAX:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r32(mvector_at(xstmt->oprs, 0))
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_EAX) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_O_R64_RAX:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r64(mvector_at(xstmt->oprs, 0))
             && _eq_reg(mvector_at(xstmt->oprs, 1), REG_RAX)) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;

    case ENC_M_MUNSPEC:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_m_unspec(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_m8(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_M16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_m16(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_M32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_m32(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_M64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_m64(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_R64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_r64(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_RM8:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_rm8(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_RM16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_rm16(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_RM32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_rm32(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;
    case ENC_M_RM64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_rm64(mvector_at(xstmt->oprs, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->oprs, 0));
        }
        break;

    case ENC_RMI_R16_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r16_rm16_imm8(mvector_at(xstmt->oprs, 0),
                                  mvector_at(xstmt->oprs, 1),
                                  mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE8);
        }
        break;
    case ENC_RMI_R16_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r16_rm16_imm16(mvector_at(xstmt->oprs, 0),
                                   mvector_at(xstmt->oprs, 1),
                                   mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE16);
        }
        break;
    case ENC_RMI_R32_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r32_rm32_imm8(mvector_at(xstmt->oprs, 0),
                                  mvector_at(xstmt->oprs, 1),
                                  mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE8);
        }
        break;
    case ENC_RMI_R32_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r32_rm32_imm32(mvector_at(xstmt->oprs, 0),
                                   mvector_at(xstmt->oprs, 1),
                                   mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r64_rm64_imm8(mvector_at(xstmt->oprs, 0),
                                  mvector_at(xstmt->oprs, 1),
                                  mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE8);
        }
        break;
    case ENC_RMI_R64_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->oprs)
             && _is_r64_rm64_imm32(mvector_at(xstmt->oprs, 0),
                                   mvector_at(xstmt->oprs, 1),
                                   mvector_at(xstmt->oprs, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->oprs, 0),
                                mvector_at(xstmt->oprs, 1),
                                mvector_at(xstmt->oprs, 2), SIZE32);
        }
        break;

    case ENC_OI_R8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r8_imm8(mvector_at(xstmt->oprs, 0),
                            mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE8);
        }
        break;
    case ENC_OI_R16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r16_imm16(mvector_at(xstmt->oprs, 0),
                              mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE16);
        }
        break;
    case ENC_OI_R32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r32_imm32(mvector_at(xstmt->oprs, 0),
                              mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE32);
        }
        break;
    case ENC_OI_R64_IMM64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->oprs)
             && _is_r64_imm64(mvector_at(xstmt->oprs, 0),
                              mvector_at(xstmt->oprs, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->oprs, 0),
                               mvector_at(xstmt->oprs, 1), SIZE64);
        }
        break;

    case ENC_D_REL8:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_d(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE8);
        }
        break;
    case ENC_D_REL16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_d(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE16);
        }
        break;
    case ENC_D_REL32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->oprs)
             && _is_imm(mvector_at(xstmt->oprs, 0), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_d(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->oprs, 0), SIZE32);
        }
        break;

    default:
        stat = 0;
    }

    return stat;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

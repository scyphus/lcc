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
_encode_rm(const x86_64_eval_t *, const x86_64_eval_t *, x86_64_enop_t *);
static int
_encode_mr(const x86_64_eval_t *, const x86_64_eval_t *, x86_64_enop_t *);
static int
_encode_oi(const x86_64_eval_t *, const x86_64_eval_t *, size_t,
           x86_64_enop_t *);
static int
_encode_mi(const x86_64_eval_t *, const x86_64_eval_t *, int, size_t,
           x86_64_enop_t *);


/*
 * Is an immediate value? (imm8, imm16, imm32, or imm64)
 */
static __inline__ int
_is_imm(x86_64_eval_t *eval, int sz)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        switch ( eval->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == eval->sopsize ) {
                /* Operand size is not specified */
                return 1;
            } else if ( sz == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( 0 == eval->sopsize )  {
                /* Operand size is not specified */
                return 1;
            } else if ( sz == eval->sopsize ) {
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
_is_imm8(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        switch ( eval->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == eval->sopsize ) {
                /* Operand size is not specified */
                if ( eval->u.imm.u.fixed >= -128
                     && eval->u.imm.u.fixed <= 127 ) {
                    /* In the range of imm8 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE8 == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE8 == eval->sopsize ) {
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
_is_imm16(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        switch ( eval->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == eval->sopsize ) {
                /* Operand size is not specified */
                if ( eval->u.imm.u.fixed >= -32768
                     && eval->u.imm.u.fixed <= 32767 ) {
                    /* In the range of imm16 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE16 == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE16 == eval->sopsize ) {
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
_is_imm32(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        switch ( eval->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == eval->sopsize ) {
                /* Operand size is not specified */
                if ( eval->u.imm.u.fixed >= -2147483648
                     && eval->u.imm.u.fixed <= 2147483647 ) {
                    /* In the range of imm32 */
                    return 1;
                } else {
                    return 0;
                }
            } else if ( SIZE32 == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE32 == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else if ( 0 == eval->sopsize ) {
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
_is_imm64(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        switch ( eval->u.imm.type ) {
        case X86_64_IMM_FIXED:
            /* Fixed value */
            if ( 0 == eval->sopsize ) {
                /* Operand size is not specified */
                return 1;
            } else if ( SIZE64 == eval->sopsize ) {
                /* Operand size is specified */
                return 1;
            } else {
                return 0;
            }
            break;
        case X86_64_IMM_EXPR:
            if ( SIZE64 == eval->sopsize ) {
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


/*
 * Is the estimated value an X-bit relative address?
 */
#if 0
static __inline__ int
_is_rel8(x86_64_eval_t *eval, off_t pos)
{
    int64_t imin;
    int64_t imax;

    if ( X86_64_EVAL_IMM == eval->type ) {
        /* Fixed value */
        if ( 0 == eval->sopsize ) {
            //imin = eval->u.imm.u.est.min - pos * X86_64_INSTR_SIZE_MAX;
            //imax = eval->u.imm.u.est.max - pos * X86_64_INSTR_SIZE_MIN;
            /* Operand size is not specified */
            if ( imin >= -128 && imax <= 127 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE8 == eval->sopsize ) {
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
_is_imm16(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type
         && X86_64_IMM_FIXED == eval->u.imm.type ) {
        /* Fixed value */
        if ( 0 == eval->sopsize ) {
            /* Operand size is not specified */
            if ( eval->u.imm.u.fixed >= -32768
                 && eval->u.imm.u.fixed <= 32767 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE16 == eval->sopsize ) {
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
_is_imm32(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type
         && X86_64_IMM_FIXED == eval->u.imm.type ) {
        /* Fixed value */
        if ( 0 == eval->sopsize ) {
            /* Operand size is not specified */
            if ( eval->u.imm.u.fixed >= -2147483648
                 && eval->u.imm.u.fixed <= 2147483647 ) {
                /* In the range of imm16 */
                return 1;
            } else {
                return 0;
            }
        } else if ( SIZE32 == eval->sopsize ) {
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
_is_imm64(x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_IMM == eval->type ) {
        if ( 0 == eval->sopsize ) {
            return 1;
        } else if ( SIZE64 == eval->sopsize ) {
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
_eq_reg(const x86_64_eval_t *eval, x86_64_reg_t reg)
{
    if ( X86_64_EVAL_REG == eval->type && reg == eval->u.reg ) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Is the register with the specified size?
 */
static __inline__ int
_is_r16(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE16 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_r32(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE32 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_r64(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE64 == eval->sopsize ) {
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
_is_m8(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE8 == eval->sopsize ) {
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
_is_rm8(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type || X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE8 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm16(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type || X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE16 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm32(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type || X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE32 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_rm64(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type || X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE64 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}





static __inline__ int
_is_reg(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg8(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE8 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg16(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE16 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg32(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE32 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg64(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_REG == eval->type ) {
        if ( SIZE64 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_addr8(const x86_64_eval_t *eval)
{
    if ( X86_64_EVAL_ADDR == eval->type ) {
        if ( SIZE8 == eval->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}



static __inline__ int
_is_rm8_r8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE8 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE8 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE32 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE8 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE64 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE8 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE16 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE16 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE32 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE16 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm32_r32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE32 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE32 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm64_r64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval2->type && SIZE64 == eval2->sopsize
         && (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && (0 == eval1->sopsize || SIZE64 == eval1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_r8_rm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm8_r8(eval2, eval1);
}
static __inline__ int
_is_r16_rm16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm16_r16(eval2, eval1);
}
static __inline__ int
_is_r32_rm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm8_r32(eval2, eval1);
}
static __inline__ int
_is_r32_rm16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm16_r32(eval2, eval1);
}
static __inline__ int
_is_r32_rm32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm32_r32(eval2, eval1);
}
static __inline__ int
_is_r64_rm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm8_r64(eval2, eval1);
}
static __inline__ int
_is_r64_rm64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    return _is_rm64_r64(eval2, eval1);
}
static __inline__ int
_is_r8_imm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE8 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize ) {
            if ( eval2->u.imm.u.fixed >= -128 && eval2->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_imm16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE16 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize ) {
            if ( eval2->u.imm.u.fixed >= -32768
                 && eval2->u.imm.u.fixed <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_imm32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE32 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize ) {
            if ( eval2->u.imm.u.fixed >= -2147483648
                 && eval2->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_imm64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE64 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize ) {
            return 1;
        } else if ( SIZE64 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r8_rela8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE8 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_EXPR == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize || SIZE8 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rela16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE16 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_EXPR == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize || SIZE16 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rela32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE32 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_EXPR == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize || SIZE32 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rela64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( X86_64_EVAL_REG == eval1->type && SIZE64 == eval1->sopsize
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_EXPR == eval2->u.imm.type ) {
        if ( 0 == eval2->sopsize || SIZE64 == eval2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_rm8_imm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( SIZE8 == eval1->sopsize ) {
            if ( 0 == eval2->sopsize ) {
                if ( eval2->u.imm.u.fixed >= -128
                     && eval2->u.imm.u.fixed <= 127 ) {
                    return 1;
                }
            } else if ( SIZE8 == eval2->sopsize ) {
                return 1;
            }
        } else if ( 0 == eval1->sopsize ) {
            if ( SIZE8 == eval2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm16_imm16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( SIZE16 == eval1->sopsize ) {
            if ( 0 == eval2->sopsize ) {
                if ( eval2->u.imm.u.fixed >= -32768
                     && eval2->u.imm.u.fixed <= 32767 ) {
                    return 1;
                }
            } else if ( SIZE16 == eval2->sopsize ) {
                return 1;
            }
        } else if ( 0 == eval1->sopsize ) {
            if ( SIZE16 == eval2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm32_imm32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( SIZE32 == eval1->sopsize ) {
            if ( 0 == eval2->sopsize ) {
                if ( eval2->u.imm.u.fixed >= -2147483648
                     && eval2->u.imm.u.fixed <= 2147483647 ) {
                    return 1;
                }
            } else if ( SIZE32 == eval2->sopsize ) {
                return 1;
            }
        } else if ( 0 == eval1->sopsize ) {
            if ( SIZE32 == eval2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm64_imm64(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2)
{
    /* Check the first and second operands */
    if ( (X86_64_EVAL_REG == eval1->type || X86_64_EVAL_ADDR == eval1->type)
         && X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        if ( SIZE64 == eval1->sopsize ) {
            if ( 0 == eval2->sopsize ) {
                return 1;
            } else if ( SIZE64 == eval2->sopsize ) {
                return 1;
            }
        } else if ( 0 == eval1->sopsize ) {
            if ( SIZE64 == eval2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
/* RMI */
static __inline__ int
_is_r16_rm16_imm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                  const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE16 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE16 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -128 && eval3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                  const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE32 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE32 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -128 && eval3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm8(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                  const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE64 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE64 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -128 && eval3->u.imm.u.fixed <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rm16_imm16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                   const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE16 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE16 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -32768
                 && eval3->u.imm.u.fixed <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rm16_rela16(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                   const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_EXPR == eval3->u.imm.type ) {
        if ( SIZE16 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE16 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize || SIZE16 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                   const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE32 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE32 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -2147483648
                 && eval3->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_rela32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                   const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_EXPR == eval3->u.imm.type ) {
        if ( SIZE32 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE32 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize || SIZE32 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                   const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        if ( SIZE64 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE64 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize ) {
            if ( eval3->u.imm.u.fixed >= -2147483648
                 && eval3->u.imm.u.fixed <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == eval3->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_rela32(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
                    const x86_64_eval_t *eval3)
{
    /* Check the type of all operands */
    if ( X86_64_EVAL_REG == eval1->type
         && (X86_64_EVAL_REG == eval2->type || X86_64_EVAL_ADDR == eval2->type)
         && X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_EXPR == eval3->u.imm.type ) {
        if ( SIZE64 != eval1->sopsize ) {
            return 0;
        }
        if ( 0 != eval2->sopsize && SIZE64 != eval2->sopsize ) {
            return 0;
        }
        if ( 0 == eval3->sopsize || SIZE32 == eval3->sopsize ) {
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
_resolve_operand_size1(const x86_64_eval_t *eval)
{
    size_t s1;
    size_t s2;

    /* Specified operand size */
    s1 = eval->sopsize;

    /* Estimated operand size */
    switch ( eval->type ) {
    case X86_64_EVAL_REG:
        s2 = regsize(eval->u.reg);
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
_resolve_address_size1(const x86_64_eval_t *eval)
{
    size_t s1;
    size_t s2;
    size_t s3;

    if ( X86_64_EVAL_ADDR != eval->type ) {
        return 0;
    }

    /* Specified address size */
    s1 = eval->u.eaddr.saddrsize;

    /* Estimated operand size */
    if ( X86_64_ADDR_BASE & eval->u.eaddr.flags ) {
        s2 = regsize(eval->u.eaddr.base);
    } else {
        s2 = 0;
    }
    if ( X86_64_ADDR_OFFSET & eval->u.eaddr.flags ) {
        s3 = regsize(eval->u.eaddr.offset);
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

static int
_build_instruction(const x86_64_asm_opt_t *opt, const x86_64_enop_t *enop,
                   size_t opsize, size_t addrsize, x86_64_instr_t *instr)
{
    int rex;
    int p66 = 0;
    int p67 = 0;
    int rexw = REX_NONE;

    switch ( opt->tgt ) {
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
    instr->disp.expr = enop->disp.expr;
    instr->imm.sz = enop->imm.sz;
    instr->imm.val = enop->imm.val;
    instr->imm.expr = enop->imm.expr;
    instr->rel.sz = enop->rel.sz;
    instr->rel.val = enop->rel.val;
    instr->rel.expr = enop->rel.expr;

    if ( OPCODE_PREFIX_LOCK & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf0;
    }
    if ( OPCODE_PREFIX_CRC32 & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REPNE & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REPNZ & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf2;
    }
    if ( OPCODE_PREFIX_REP & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_REPE & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_REPZ & opt->prefix ) {
        if ( instr->prefix1 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix1 = 0xf3;
    }
    if ( OPCODE_PREFIX_BRANCH_NOT_TAKEN & opt->prefix ) {
        if ( instr->prefix3 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix3 = 0x2e;
    }
    if ( OPCODE_PREFIX_BRANCH_TAKEN & opt->prefix ) {
        if ( instr->prefix3 >= 0 ) {
            return -EPREFIX;
        }
        instr->prefix3 = 0x3e;
    }

    return 0;
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
_encode_scaled_index(const x86_64_eval_t *eval, int *idx, int *rexx, int *ss)
{
    int ret;

    assert( X86_64_EVAL_ADDR == eval->type );

    /* Encode scaled index */
    if ( X86_64_ADDR_OFFSET & eval->u.eaddr.flags ) {
        /* Resolve the register code of the index register */
        ret = _reg_code(eval->u.eaddr.offset, idx, rexx);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }
        if ( 4 == *idx && (REX_NONE == *rexx || REX_FALSE == *rexx) ) {
            /* Invalid index register */
            return -1;
        }

        /* Check scale multiplier */
        if ( X86_64_ADDR_SCALE & eval->u.eaddr.flags ) {
            switch ( eval->u.eaddr.scale ) {
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
_encode_disp(const x86_64_eval_t *eval, int64_t *disp, size_t *dispsz)
{
    assert( X86_64_EVAL_ADDR == eval->type );

    if ( eval->u.eaddr.flags & X86_64_ADDR_DISP ) {
        /* Displacement is specified */
        if ( X86_64_IMM_FIXED == eval->u.eaddr.disp.type ) {
            /* Get the value of displacement */
            *disp = eval->u.eaddr.disp.u.fixed;
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
_encode_rm_addr_with_base(int reg, int rexr, const x86_64_eval_t *eval,
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

    assert( X86_64_EVAL_ADDR == eval->type );
    assert( X86_64_ADDR_BASE & eval->u.eaddr.flags );

    rexx = REX_NONE;
    rexb = REX_NONE;

    /* Is the base register EIP/RIP? */
    if ( REG_EIP == eval->u.eaddr.base || REG_RIP == eval->u.eaddr.base ) {
        /* The base register is EIP/RIP */
        if ( (X86_64_ADDR_OFFSET & eval->u.eaddr.flags)
             || (X86_64_ADDR_SCALE & eval->u.eaddr.flags) ) {
            /* Must not have scaled index */
            return -1;
        }
        mod = 0;
        rm = 5;

        /* Encode ModR/M */
        modrm = _encode_modrm(reg, mod, rm);
        sib = -1;

        /* Encode displacement */
        ret = _encode_disp(eval, &disp, &dispsz);
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
        enop->disp.eval = eval;
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

        return 0;
    } else {
        /* The base register is not EIP/RIP and needs to specify SIB */

        /* Resolve the register code of the base register */
        ret = _reg_code(eval->u.eaddr.base, &base, &rexb);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }

        /* Encode displacement */
        ret = _encode_disp(eval, &disp, &dispsz);
        if ( ret < 0 ) {
            return -1;
        }

        if ( X86_64_ADDR_OFFSET & eval->u.eaddr.flags ) {
            /* Requires SIB */
            ret = _encode_scaled_index(eval, &idx, &rexx, &ss);
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
        enop->disp.eval = eval;
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

        return 0;
    }
}

/*
 * Encode the second operand with the addr type for the w/o base register case
 */
static int
_encode_rm_addr_without_base(int reg, int rexr, const x86_64_eval_t *eval,
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

    assert( X86_64_EVAL_ADDR == eval->type );
    assert( !(X86_64_ADDR_BASE & eval->u.eaddr.flags) );

    /* Mod and R/M must be 0 and 4, respectively, for the case w/o base
       register */
    mod = 0;
    rm = 4;
    /* The base register 5 where Mod is 5 means "w/o base register" */
    base = 5;

    ret = _encode_scaled_index(eval, &idx, &rexx, &ss);
    if ( ret < 0 ) {
        return -1;
    }

    /* Encode displacement */
    ret = _encode_disp(eval, &disp, &dispsz);
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
    enop->disp.eval = eval;
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

    return 0;
}

/*
 * Encode the second R/M of RM operands
 */
static int
_encode_rm_addr(int reg, int rexr, const x86_64_eval_t *eval,
                x86_64_enop_t *enop)
{
    int ret;

    assert( X86_64_EVAL_ADDR == eval->type );

    if ( X86_64_ADDR_BASE & eval->u.eaddr.flags ) {
        /* With base register */
        ret = _encode_rm_addr_with_base(reg, rexr, eval, enop);
    } else {
        /* Without base register */
        ret = _encode_rm_addr_without_base(reg, rexr, eval, enop);
    }

    return 0;
}

/*
 * Encode the second register of RM operands
 */
static int
_encode_rm_reg(int reg, int rexr, const x86_64_eval_t *eval,
               x86_64_enop_t *enop)
{
    int ret;
    int mod;
    int rm;
    int rexb;
    int modrm;

    assert( X86_64_EVAL_REG == eval->type );

    /* Mod = 3 where  R/M is register */
    mod = 3;

    /* Resolve the register code of the second operand */
    ret = _reg_code(eval->u.reg, &rm, &rexb);
    if ( ret < 0 ) {
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

    return 0;
}

/*
 * Encode operands whose Op/En is RM
 */
static int
_encode_rm(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_EVAL_REG != eval1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(eval1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_EVAL_REG == eval2->type ) {
        ret = _encode_rm_reg(reg, rexr, eval2, enop);
        if ( ret < 0 ) {
            return -1;
        }

        return 0;
    } else if ( X86_64_EVAL_ADDR == eval2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, eval2, enop);
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
_encode_mr(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
           x86_64_enop_t *enop)
{
    return _encode_rm(eval2, eval1, enop);
}

/*
 * Encode operands whose Op/En is RMI
 */
static int
_encode_rmi(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2,
            const x86_64_eval_t *eval3, size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_EVAL_REG != eval1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(eval1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_EVAL_REG == eval2->type ) {
        ret = _encode_rm_reg(reg, rexr, eval2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_EVAL_ADDR == eval2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, eval2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    /* Check the third operand */
    if ( X86_64_EVAL_IMM == eval3->type
         && X86_64_IMM_FIXED == eval3->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = eval3->u.imm.u.fixed;
        enop->imm.eval = NULL;
    } else if ( X86_64_EVAL_IMM == eval3->type
                && X86_64_IMM_EXPR == eval3->u.imm.type ) {
        /* Update relocatable value */
        enop->imm.sz = immsz;
        enop->imm.val = 0;
        enop->imm.eval = eval3;
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
_encode_oi(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2, size_t immsz,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_EVAL_REG != eval1->type ) {
        return -1;
    }
    ret = _reg_code(eval1->u.reg, &reg, &rexb);
    if ( ret < 0 ) {
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
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
        enop->imm.val = eval2->u.imm.u.fixed;
        enop->imm.eval = NULL;
        enop->imm.expr = NULL;
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;

        return 0;
    } else if ( X86_64_EVAL_IMM == eval2->type
                && X86_64_IMM_EXPR == eval2->u.imm.type ) {
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
        enop->imm.eval = eval2;
        /* FIXME */
        enop->imm.expr = NULL;
        enop->rel.sz = 0;
        enop->rel.val = 0;
        enop->rel.expr = NULL;

        return 0;
    } else {
        return -1;
    }
}

/*
 * Encode the MI type Op/En
 */
static int
_encode_mi(const x86_64_eval_t *eval1, const x86_64_eval_t *eval2, int reg,
           size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_EVAL_REG == eval1->type ) {
        ret = _encode_rm_reg(reg, rexr, eval1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_EVAL_ADDR == eval1->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, eval1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }


    /* Check the second operand */
    if ( X86_64_EVAL_IMM == eval2->type
         && X86_64_IMM_FIXED == eval2->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = eval2->u.imm.u.fixed;
        enop->imm.expr = NULL;
    } else if ( X86_64_EVAL_IMM == eval2->type
                && X86_64_IMM_EXPR == eval2->u.imm.type ) {
        /* Update immediate value */
        enop->imm.sz = immsz;
        enop->imm.val = 0;
        enop->imm.expr = eval2->u.imm.u.rexpr;
    } else {
        return -1;
    }

    return 0;
}
static int
_encode_m(const x86_64_eval_t *eval, int reg, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_EVAL_REG == eval->type ) {
        ret = _encode_rm_reg(reg, rexr, eval, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_EVAL_ADDR == eval->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_addr(reg, rexr, eval, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}
static int
_encode_o(const x86_64_eval_t *eval, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_EVAL_REG != eval->type ) {
        return -1;
    }
    ret = _reg_code(eval->u.reg, &reg, &rexb);
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

    return 0;
}

/*
 * Encode the I type Op/En
 */
static int
_encode_i(const x86_64_eval_t *eval, size_t immsz, x86_64_enop_t *enop)
{
    /* Check the operand type */
    if ( X86_64_EVAL_IMM == eval->type ) {
        if ( X86_64_IMM_FIXED == eval->u.imm.type ) {
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
            enop->imm.val = eval->u.imm.u.fixed;
            enop->imm.expr = NULL;
            enop->rel.sz = 0;
            enop->rel.val = 0;
            enop->rel.expr = NULL;
            return 0;
        } else if ( X86_64_IMM_EXPR == eval->u.imm.type ) {
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
            enop->imm.expr = eval->u.imm.u.rexpr;
            enop->rel.sz = 0;
            enop->rel.val = 0;
            enop->rel.expr = NULL;
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
_encode_d(const x86_64_eval_t *eval, size_t immsz, x86_64_enop_t *enop)
{
    /* FIXME!!! */
    return -1;
}


/*
 * Evaluate operands
 */
static int
_eval1(const x86_64_label_table_t *ltbl, x86_64_eval_t **eval,
       const operand_vector_t *operands)
{
    operand_t *op;

    /* Check the number of operand */
    if ( 1 != mvector_size(operands) ) {
        return 0;
    }
    /* Obtain operands */
    op = mvector_at(operands, 0);
    /* Evaluate operands */
    *eval = x86_64_estimate_operand(op);
    if ( NULL == *eval ) {
        /* Error */
        return -1;
    }

    return 1;
}
static int
_eval2(const x86_64_label_table_t *ltbl, x86_64_eval_t **eval1,
       x86_64_eval_t **eval2, const operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;

    /* Check the number of operand */
    if ( 2 != mvector_size(operands) ) {
        return 0;
    }
    /* Obtain operands */
    op1 = mvector_at(operands, 0);
    op2 = mvector_at(operands, 1);
    /* Evaluate operands */
    *eval1 = x86_64_estimate_operand(op1);
    if ( NULL == *eval1 ) {
        /* Error */
        return -1;
    }
    *eval2 = x86_64_estimate_operand(op2);
    if ( NULL == *eval2 ) {
        /* Error */
        free(*eval1);
        return -1;
    }

    return 1;
}
static int
_eval3(const x86_64_label_table_t *ltbl, x86_64_eval_t **eval1,
       x86_64_eval_t **eval2, x86_64_eval_t **eval3,
       const operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    operand_t *op3;

    /* Check the number of operand */
    if ( 3 != mvector_size(operands) ) {
        return 0;
    }
    /* Obtain operands */
    op1 = mvector_at(operands, 0);
    op2 = mvector_at(operands, 1);
    op3 = mvector_at(operands, 2);
    /* Evaluate operands */
    *eval1 = x86_64_estimate_operand(op1);
    if ( NULL == *eval1 ) {
        /* Error */
        return -1;
    }
    *eval2 = x86_64_estimate_operand(op2);
    if ( NULL == *eval2 ) {
        /* Error */
        free(*eval1);
        return -1;
    }
    *eval3 = x86_64_estimate_operand(op3);
    if ( NULL == *eval3 ) {
        /* Error */
        free(*eval1);
        free(*eval2);
        return -1;
    }

    return 1;
}
static int
_eval4(const x86_64_label_table_t *ltbl, x86_64_eval_t **eval1,
       x86_64_eval_t **eval2, x86_64_eval_t **eval3, x86_64_eval_t **eval4,
       const operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    operand_t *op3;
    operand_t *op4;

    /* Check the number of operand */
    if ( 4 != mvector_size(operands) ) {
        return 0;
    }
    /* Obtain operands */
    op1 = mvector_at(operands, 0);
    op2 = mvector_at(operands, 1);
    op3 = mvector_at(operands, 2);
    op4 = mvector_at(operands, 3);
    /* Evaluate operands */
    *eval1 = x86_64_estimate_operand(op1);
    if ( NULL == *eval1 ) {
        /* Error */
        return -1;
    }
    *eval2 = x86_64_estimate_operand(op2);
    if ( NULL == *eval2 ) {
        /* Error */
        free(*eval1);
        return -1;
    }
    *eval3 = x86_64_estimate_operand(op3);
    if ( NULL == *eval3 ) {
        /* Error */
        free(*eval1);
        free(*eval2);
        return -1;
    }
    *eval4 = x86_64_estimate_operand(op4);
    if ( NULL == *eval4 ) {
        /* Error */
        free(*eval1);
        free(*eval2);
        free(*eval3);
        return -1;
    }

    return 1;
}
static int
_eval(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    x86_64_eval_t *eval;
    operand_t *op;
    size_t nr;
    size_t i;
    x86_64_eval_vector_t *evals;

    /* Allocate a vector for evals */
    evals = mvector_new();
    if ( NULL == evals ) {
        return -1;
    }

    /* Evaluate operands */
    nr = mvector_size(xstmt->stmt->u.instr->operands);
    for ( i = 0; i < nr; i++ ) {
        /* Obtain operands */
        op = mvector_at(xstmt->stmt->u.instr->operands, i);
        /* Evaluate operands */
        eval = x86_64_estimate_operand(op);
        if ( NULL == eval ) {
            /* FIXME: Free the contents of the vector */
            mvector_delete(evals);
            /* Error */
            return -EOPERAND;
        }
        if ( NULL == mvector_push_back(evals, eval) ) {
            free(eval);
            /* FIXME: Free the contents of the vector */
            mvector_delete(evals);
            /* Error */
            return -EUNKNOWN;
        }
    }

    /* Set evals */
    xstmt->evals = evals;

    return 1;
}


/*
 * Build instruction for the NP type Op/En
 */
static int
_binstr_np(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, int opsize)
{
    int ret;
    x86_64_enop_t enop;

    if ( opsize < 0 ) {
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
    enop.disp.eval = NULL;
    enop.imm.sz = 0;
    enop.imm.val = 0;
    enop.imm.eval = NULL;
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, 0, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

/*
 * Build instruction for the I type Op/En
 */
static int
_binstr_i(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
          int opc2, int opc3, size_t opsize, const x86_64_eval_t *eval,
          size_t immsz)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_i(eval, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, 0, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    /* Set opcode */
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

/*
 * Build instruction for the D type Op/En
 */
static int
_binstr_d(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
          int opc2, int opc3, size_t opsize, const x86_64_eval_t *eval,
          size_t immsz)
{
    /* FIXME!!! */
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_d(eval, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, 0, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

/*
 * Build instruction for the MI type Op/En
 */
static int
_binstr_mi(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, int preg, size_t opsize,
           const x86_64_eval_t *evalm, const x86_64_eval_t *evali, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Check pseudo register */
    if ( preg < 0 ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_mi(evalm, evali, preg, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
    if ( addrsize < 0 ) {
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

static int
_binstr_mr(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, size_t opsize, const x86_64_eval_t *evalm,
           const x86_64_eval_t *evalr)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_mr(evalm, evalr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
    if ( addrsize < 0 ) {
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

static int
_binstr_rm(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, size_t opsize, const x86_64_eval_t *evalr,
           const x86_64_eval_t *evalm)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_rm(evalr, evalm, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
    if ( addrsize < 0 ) {
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}
static int
_binstr_rmi(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
            int opc2, int opc3, size_t opsize, const x86_64_eval_t *evalr,
            const x86_64_eval_t *evalm, const x86_64_eval_t *evali,
            size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_rmi(evalr, evalm, evali, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
    if ( addrsize < 0 ) {
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}


static int
_binstr_o(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
          int opc2, int opc3, size_t opsize, const x86_64_eval_t *evalr)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_o(evalr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, 0, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
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

    /* Success */
    return 1;
}

static int
_binstr_oi(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, size_t opsize, const x86_64_eval_t *evalr,
           const x86_64_eval_t *evali, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_oi(evalr, evali, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, 0, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
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

    /* Success */
    return 1;
}

static int
_binstr_m(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
          int opc2, int opc3, int preg, size_t opsize,
          const x86_64_eval_t *evalm)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_m(evalm, preg, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
    if ( addrsize < 0 ) {
        return -ESIZE;
    }
    /* Build instruction */
    ret = _build_instruction(opt, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        /* Invalid operands */
        return -EOPERAND;
    }
    instr->opcode1 = opc1;
    instr->opcode2 = opc2;
    instr->opcode3 = opc3;

    /* Success */
    return 1;
}

/*
 * Build instruction and return a success/error code
 */
int
binstr(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opsize, int opc1,
       int opc2, int opc3, int preg, const operand_vector_t *operands,
       x86_64_enc_t enc)
{
    int i;
    int nr;
    int ret;
    int stat;
    x86_64_eval_t *eval[4];

    /* Obtain the number of operands */
    nr = mvector_size(operands);
    if ( 1 == nr ) {
        /* Evaluate operands */
        ret = _eval1(opt->ltbl, &eval[0], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
   } else if ( 2 == nr ) {
        /* Evaluate operands */
        ret = _eval2(opt->ltbl, &eval[0], &eval[1], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
    } else if ( 3 == nr ) {
        /* Evaluate operands */
        ret = _eval3(opt->ltbl, &eval[0], &eval[1], &eval[2], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
    } else if ( 4 == nr ) {
        /* Evaluate operands */
        ret = _eval4(opt->ltbl, &eval[0], &eval[1], &eval[2], &eval[3],
                     operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
    }

    stat = 0;
    switch ( enc ) {
    case ENC_NP:
        /* Check the number of operands */
        if ( 0 == nr ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AL_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AL) && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX) && _eq_reg(eval[1], REG_AL) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX) && _eq_reg(eval[1], REG_AX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_EAX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX)
             && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX)
             && _eq_reg(eval[1], REG_EAX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_I_FIMM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_imm16(eval[0]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE16);
        }
        break;
    case ENC_I_AL_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AL) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _is_imm16(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE16);
        }
        break;
    case ENC_I_EAX_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_EAX_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX) && _is_imm32(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE32);
        }
        break;
    case ENC_I_RAX_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_RAX) && _is_imm32(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE32);
        }
        break;
    case ENC_I_FIMM8_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_AL) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_I_FIMM8_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_AX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_I_FIMM8_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_EAX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
#if 0
    case ENC_D_REL8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel8(eval[0], opt->pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_D_REL16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel16(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE16);
        }
        break;
    case ENC_D_REL32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel32(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE32);
        }
        break;
    case ENC_D_REL32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_erel32(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE32);
        }
        break;
#endif
    case ENC_MI_RM8_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16(eval[0]) && _is_imm16(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_MI_RM32_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM32_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32(eval[0]) && _is_imm32(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM64_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm32(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_FIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm64(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8_r8(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16_r16(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32_r32(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64_r64(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE8, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rm16(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE16, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm16(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm32(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE32, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm64(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE64, eval[0],
                              eval[1]);
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg32(eval[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, eval[0]);
        }
        break;
    case ENC_O_R64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg64(eval[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, eval[0]);
        }
        break;
    case ENC_OI_R8_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_imm8(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_OI_R8_EIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rela8(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_OI_R16_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_imm16(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_OI_R16_EIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rela16(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_OI_R32_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_imm32(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_OI_R32_EIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rela32(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_OI_R64_FIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_imm64(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_OI_R64_EIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rela64(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_addr8(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, SIZE8,
                             eval[0]);
        }
        break;
    case ENC_M_RM8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm8(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_M_RM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm16(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
    case ENC_M_RM32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm32(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_M_RM64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm64(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_RMI_R16_RM16_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R16_RM16_FIMM16:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm16(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE16);
        }
        break;
    case ENC_RMI_R16_RM16_EIMM16:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_rela16(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE16);
        }
        break;
    case ENC_RMI_R32_RM32_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R32_RM32_FIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R32_RM32_EIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_rela32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R64_RM64_FIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_EIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_rela32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    default:
        stat = 0;
    }

    /* Free values */
    for ( i = 0; i < nr; i++ ) {
        free(eval[i]);
    }

    return stat;
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
           const x86_64_eval_t *eval, size_t immsz)
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
    ret = _encode_i(eval, immsz, &enop);
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
            ssize_t opsize, const x86_64_eval_t *evalm,
            const x86_64_eval_t *evali, size_t immsz)
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
    ret = _encode_mi(evalm, evali, preg, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }

    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
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
            const x86_64_eval_t *evalm, const x86_64_eval_t *evalr)
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
    ret = _encode_mr(evalm, evalr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
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
           const x86_64_eval_t *evalr)
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
    ret = _encode_o(evalr, &enop);
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
            const x86_64_eval_t *evalr, const x86_64_eval_t *evalm)
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
    ret = _encode_rm(evalr, evalm, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
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
           ssize_t opsize, const x86_64_eval_t *evalm)
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
    ret = _encode_m(evalm, preg, &enop);
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
             const x86_64_eval_t *evalr, const x86_64_eval_t *evalm,
             const x86_64_eval_t *evali, size_t immsz)
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
    ret = _encode_rmi(evalr, evalm, evali, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        free(instr);
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(evalm);
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
            const x86_64_eval_t *evalr, const x86_64_eval_t *evali,
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
    ret = _encode_oi(evalr, evali, immsz, &enop);
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
        if ( 0 == mvector_size(xstmt->evals) ) {
            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AL_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_AL)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AL:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_AL) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AX_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_AX)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_AX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_EAX_DX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_EAX)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_DX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_EAX:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_DX)
             && _eq_reg(mvector_at(xstmt->evals, 1), REG_EAX) ) {

            /* Build the instruction */
            stat = _binstr2_np(xstmt, opc1, opc2, opc3, opsize);
        }
        break;

    case ENC_I_AL_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_AL)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_I_AX_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_AX)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_I_AX_IMM16:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_AX)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE16);
        }
        break;
    case ENC_I_EAX_IMM8:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_EAX)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_I_EAX_IMM32:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_EAX)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE32);
        }
        break;
    case ENC_I_RAX_IMM32:
        /* Check the number of operands and the format */
        if ( 2 == mvector_size(xstmt->evals)
             && _eq_reg(mvector_at(xstmt->evals, 0), REG_RAX)
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_i(xstmt, opc1, opc2, opc3, opsize,
                              mvector_at(xstmt->evals, 1), SIZE32);
        }
        break;

    case ENC_MI_RM8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm8(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_MI_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm16(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE16) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE16);
        }
        break;
    case ENC_MI_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm32(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE32);
        }
        break;
    case ENC_MI_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm64(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE32) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE32);
        }
        break;
    case ENC_MI_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm16(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_MI_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm32(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_MI_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm64(mvector_at(xstmt->evals, 0))
             && _is_imm(mvector_at(xstmt->evals, 1), SIZE8) ) {

            /* Build the instruction */
            stat = _binstr2_mi(xstmt, opc1, opc2, opc3, preg, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;

    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm8_r8(mvector_at(xstmt->evals, 0),
                           mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm16_r16(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm32_r32(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_rm64_r64(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_mr(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;

    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r8_rm8(mvector_at(xstmt->evals, 0),
                           mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r16_rm16(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r32_rm8(mvector_at(xstmt->evals, 0),
                            mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r32_rm16(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r32_rm32(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r64_rm8(mvector_at(xstmt->evals, 0),
                            mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r64_rm64(mvector_at(xstmt->evals, 0),
                             mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_rm(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1));
        }
        break;

    case ENC_O_R16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_r16(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_r32(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_O_R64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_r64(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_o(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0));
        }
        break;

    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_m8(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_M_RM8:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_rm8(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_M_RM16:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_rm16(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_M_RM32:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_rm32(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->evals, 0));
        }
        break;
    case ENC_M_RM64:
        /* Check the number of operands and format */
        if ( 1 == mvector_size(xstmt->evals)
             && _is_rm64(mvector_at(xstmt->evals, 0)) ) {

            /* Build the instruction */
            stat = _binstr2_m(xstmt, opc1, opc2, opc3, preg, opsize,
                              mvector_at(xstmt->evals, 0));
        }
        break;

    case ENC_RMI_R16_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r16_rm16_imm8(mvector_at(xstmt->evals, 0),
                                  mvector_at(xstmt->evals, 1),
                                  mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE8);
        }
        break;
    case ENC_RMI_R16_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r16_rm16_imm16(mvector_at(xstmt->evals, 0),
                                   mvector_at(xstmt->evals, 1),
                                   mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE16);
        }
        break;
    case ENC_RMI_R32_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r32_rm32_imm8(mvector_at(xstmt->evals, 0),
                                  mvector_at(xstmt->evals, 1),
                                  mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE8);
        }
        break;
    case ENC_RMI_R32_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r32_rm32_imm32(mvector_at(xstmt->evals, 0),
                                   mvector_at(xstmt->evals, 1),
                                   mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r64_rm64_imm8(mvector_at(xstmt->evals, 0),
                                  mvector_at(xstmt->evals, 1),
                                  mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE8);
        }
        break;
    case ENC_RMI_R64_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 3 == mvector_size(xstmt->evals)
             && _is_r64_rm64_imm32(mvector_at(xstmt->evals, 0),
                                   mvector_at(xstmt->evals, 1),
                                   mvector_at(xstmt->evals, 2)) ) {

            /* Build the instruction */
            stat = _binstr2_rmi(xstmt, opc1, opc2, opc3, opsize,
                                mvector_at(xstmt->evals, 0),
                                mvector_at(xstmt->evals, 1),
                                mvector_at(xstmt->evals, 2), SIZE32);
        }
        break;

    case ENC_OI_R8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r8_imm8(mvector_at(xstmt->evals, 0),
                            mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE8);
        }
        break;
    case ENC_OI_R16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r16_imm16(mvector_at(xstmt->evals, 0),
                              mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE16);
        }
        break;
    case ENC_OI_R32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r32_imm32(mvector_at(xstmt->evals, 0),
                              mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE32);
        }
        break;
    case ENC_OI_R64_IMM64:
        /* Check the number of operands and format */
        if ( 2 == mvector_size(xstmt->evals)
             && _is_r64_imm64(mvector_at(xstmt->evals, 0),
                              mvector_at(xstmt->evals, 1)) ) {

            /* Build the instruction */
            stat = _binstr2_oi(xstmt, opc1, opc2, opc3, opsize,
                               mvector_at(xstmt->evals, 0),
                               mvector_at(xstmt->evals, 1), SIZE64);
        }
        break;

    default:
        stat = 0;
    }

    return stat;

#if 0
    int i;


    stat = 0;
    switch ( enc ) {
    case ENC_NP:
        /* Check the number of operands */
        if ( 0 == nr ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AL_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AL) && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX) && _eq_reg(eval[1], REG_AL) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX) && _eq_reg(eval[1], REG_AX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_EAX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX)
             && _eq_reg(eval[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_DX)
             && _eq_reg(eval[1], REG_EAX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_I_FIMM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_imm16(eval[0]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE16);
        }
        break;
    case ENC_I_AL_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AL) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_AX) && _is_imm16(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE16);
        }
        break;
    case ENC_I_EAX_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX) && _is_imm8(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE8);
        }
        break;
    case ENC_I_EAX_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_EAX) && _is_imm32(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE32);
        }
        break;
    case ENC_I_RAX_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(eval[0], REG_RAX) && _is_imm32(eval[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[1],
                             SIZE32);
        }
        break;
    case ENC_I_FIMM8_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_AL) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_I_FIMM8_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_AX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_I_FIMM8_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(eval[0]) && _eq_reg(eval[1], REG_EAX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
#if 0
    case ENC_D_REL8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel8(eval[0], opt->pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE8);
        }
        break;
    case ENC_D_REL16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel16(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE16);
        }
        break;
    case ENC_D_REL32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rel32(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE32);
        }
        break;
    case ENC_D_REL32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_erel32(eval[0], pos) ) {
            stat = _binstr_d(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                             SIZE32);
        }
        break;
#endif
    case ENC_MI_RM8_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16(eval[0]) && _is_imm16(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_MI_RM32_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM32_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32(eval[0]) && _is_imm32(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm8(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_MI_RM64_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm32(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_FIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64(eval[0]) && _is_imm64(eval[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, opsize,
                              eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8_r8(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16_r16(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32_r32(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64_r64(eval[0], eval[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE8, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rm16(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE16, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm16(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm32(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE32, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm8(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, eval[0],
                              eval[1]);
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm64(eval[0], eval[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE64, eval[0],
                              eval[1]);
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg32(eval[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, eval[0]);
        }
        break;
    case ENC_O_R64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg64(eval[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, eval[0]);
        }
        break;
    case ENC_OI_R8_FIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_imm8(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_OI_R8_EIMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rela8(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE8);
        }
        break;
    case ENC_OI_R16_FIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_imm16(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_OI_R16_EIMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rela16(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE16);
        }
        break;
    case ENC_OI_R32_FIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_imm32(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_OI_R32_EIMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rela32(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE32);
        }
        break;
    case ENC_OI_R64_FIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_imm64(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_OI_R64_EIMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rela64(eval[0], eval[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  eval[0], eval[1], SIZE64);
        }
        break;
    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_addr8(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, SIZE8,
                             eval[0]);
        }
        break;
    case ENC_M_RM8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm8(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_M_RM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm16(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
    case ENC_M_RM32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm32(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_M_RM64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_rm64(eval[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             eval[0]);
        }
        break;
    case ENC_RMI_R16_RM16_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R16_RM16_FIMM16:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm16(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE16);
        }
        break;
    case ENC_RMI_R16_RM16_EIMM16:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_rela16(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE16);
        }
        break;
    case ENC_RMI_R32_RM32_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R32_RM32_FIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R32_RM32_EIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_rela32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_FIMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm8(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE8);
        }
        break;
    case ENC_RMI_R64_RM64_FIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_EIMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_rela32(eval[0], eval[1], eval[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               eval[0], eval[1], eval[2], SIZE32);
        }
        break;
    default:
        stat = 0;
    }

    /* Free values */
    for ( i = 0; i < nr; i++ ) {
        free(eval[i]);
    }

    return stat;
#endif

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

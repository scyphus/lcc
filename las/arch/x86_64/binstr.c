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
_encode_rm(const x86_64_val_t *, const x86_64_val_t *, x86_64_enop_t *);
static int
_encode_mr(const x86_64_val_t *, const x86_64_val_t *, x86_64_enop_t *);
static int
_encode_oi(const x86_64_val_t *, const x86_64_val_t *, size_t, x86_64_enop_t *);
static int
_encode_mi(const x86_64_val_t *, const x86_64_val_t *, int, size_t,
           x86_64_enop_t *);


static __inline__ int
_is_reg(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg8(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type ) {
        if ( SIZE8 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg16(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type ) {
        if ( SIZE16 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg32(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type ) {
        if ( SIZE32 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg64(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type ) {
        if ( SIZE64 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_addr8(const x86_64_val_t *val)
{
    if ( X86_64_VAL_ADDR == val->type ) {
        if ( SIZE8 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg_addr(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type || X86_64_VAL_ADDR == val->type ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg_addr8(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type || X86_64_VAL_ADDR == val->type ) {
        if ( SIZE8 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg_addr16(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type || X86_64_VAL_ADDR == val->type ) {
        if ( SIZE16 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg_addr32(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type || X86_64_VAL_ADDR == val->type ) {
        if ( SIZE32 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_is_reg_addr64(const x86_64_val_t *val)
{
    if ( X86_64_VAL_REG == val->type || X86_64_VAL_ADDR == val->type ) {
        if ( SIZE64 == val->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static __inline__ int
_eq_reg(const x86_64_val_t *val, x86_64_reg_t reg)
{
    if ( X86_64_VAL_REG == val->type && reg == val->u.reg ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm8(x86_64_val_t *val)
{
    if ( X86_64_VAL_IMM != val->type ) {
        return 0;
    } else if ( 0 == val->sopsize ) {
        if ( val->u.imm >= -128 && val->u.imm <= 127 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE8 == val->sopsize ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm16(x86_64_val_t *val)
{
    if ( X86_64_VAL_IMM != val->type ) {
        return 0;
    } else if ( 0 == val->sopsize ) {
        if ( val->u.imm >= -32768 && val->u.imm <= 32767 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE16 == val->sopsize ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm32(x86_64_val_t *val)
{
    if ( X86_64_VAL_IMM != val->type ) {
        return 0;
    } else if ( 0 == val->sopsize ) {
        if ( val->u.imm >= -2147483648 && val->u.imm <= 2147483647 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE32 == val->sopsize ) {
        return 1;
    } else {
        return 0;
    }
}
static __inline__ int
_is_imm64(x86_64_val_t *val)
{
    if ( X86_64_VAL_IMM != val->type ) {
        return 0;
    } else if ( 0 == val->sopsize ) {
        return 1;
    } else if ( SIZE64 == val->sopsize ) {
        return 1;
    } else {
        return 0;
    }
}

static __inline__ int
_is_rm8_r8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE8 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE8 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE32 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE8 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm8_r64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE64 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE8 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE16 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE16 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE32 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE16 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm32_r32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE32 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE32 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm64_r64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE64 == val2->sopsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->sopsize || SIZE64 == val1->sopsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_r8_rm8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm8_r8(val2, val1);
}
static __inline__ int
_is_r16_rm16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm16_r16(val2, val1);
}
static __inline__ int
_is_r32_rm8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm8_r32(val2, val1);
}
static __inline__ int
_is_r32_rm16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm16_r32(val2, val1);
}
static __inline__ int
_is_r32_rm32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm32_r32(val2, val1);
}
static __inline__ int
_is_r64_rm8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm8_r64(val2, val1);
}
static __inline__ int
_is_r64_rm64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm64_r64(val2, val1);
}
static __inline__ int
_is_r8_imm8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE8 == val1->sopsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->sopsize ) {
            if ( val2->u.imm >= -128 && val2->u.imm <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_imm16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE16 == val1->sopsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->sopsize ) {
            if ( val2->u.imm >= -32768 && val2->u.imm <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == val2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_imm32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE32 == val1->sopsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->sopsize ) {
            if ( val2->u.imm >= -2147483648 && val2->u.imm <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == val2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_imm64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE64 == val1->sopsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->sopsize ) {
            return 1;
        } else if ( SIZE64 == val2->sopsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_rm8_imm8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && X86_64_VAL_IMM == val2->type ) {
        if ( SIZE8 == val1->sopsize ) {
            if ( 0 == val2->sopsize ) {
                if ( val2->u.imm >= -128 && val2->u.imm <= 127 ) {
                    return 1;
                }
            } else if ( SIZE8 == val2->sopsize ) {
                return 1;
            }
        } else if ( 0 == val1->sopsize ) {
            if ( SIZE8 == val2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm16_imm16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && X86_64_VAL_IMM == val2->type ) {
        if ( SIZE16 == val1->sopsize ) {
            if ( 0 == val2->sopsize ) {
                if ( val2->u.imm >= -32768 && val2->u.imm <= 32767 ) {
                    return 1;
                }
            } else if ( SIZE16 == val2->sopsize ) {
                return 1;
            }
        } else if ( 0 == val1->sopsize ) {
            if ( SIZE16 == val2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm32_imm32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && X86_64_VAL_IMM == val2->type ) {
        if ( SIZE32 == val1->sopsize ) {
            if ( 0 == val2->sopsize ) {
                if ( val2->u.imm >= -2147483648 && val2->u.imm <= 2147483647 ) {
                    return 1;
                }
            } else if ( SIZE32 == val2->sopsize ) {
                return 1;
            }
        } else if ( 0 == val1->sopsize ) {
            if ( SIZE32 == val2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
static __inline__ int
_is_rm64_imm64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && X86_64_VAL_IMM == val2->type ) {
        if ( SIZE64 == val1->sopsize ) {
            if ( 0 == val2->sopsize ) {
                return 1;
            } else if ( SIZE64 == val2->sopsize ) {
                return 1;
            }
        } else if ( 0 == val1->sopsize ) {
            if ( SIZE64 == val2->sopsize ) {
                return 1;
            }
        }
    }

    return 0;
}
/* RMI */
static __inline__ int
_is_r16_rm16_imm8(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE16 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE16 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -128 && val3->u.imm <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm8(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE32 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE32 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -128 && val3->u.imm <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm8(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE64 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE64 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -128 && val3->u.imm <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_rm16_imm16(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE16 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE16 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -32768 && val3->u.imm <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == val2->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_rm32_imm32(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE32 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE32 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -2147483648 && val3->u.imm <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_rm64_imm32(const x86_64_val_t *val1, const x86_64_val_t *val2,
                  const x86_64_val_t *val3)
{
    /* Check the type of all operands */
    if ( X86_64_VAL_REG == val1->type
         && (X86_64_VAL_REG == val2->type || X86_64_VAL_ADDR == val2->type)
         && X86_64_VAL_IMM == val3->type ) {
        if ( SIZE64 != val1->sopsize ) {
            return 0;
        }
        if ( 0 != val2->sopsize && SIZE64 != val2->sopsize ) {
            return 0;
        }
        if ( 0 == val3->sopsize ) {
            if ( val3->u.imm >= -2147483648 && val3->u.imm <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->sopsize ) {
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
_resolve_operand_size1(const x86_64_val_t *val)
{
    size_t s1;
    size_t s2;

    /* Specified operand size */
    s1 = val->sopsize;

    /* Estimated operand size */
    switch ( val->type ) {
    case X86_64_VAL_REG:
        s2 = regsize(val->u.reg);
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
_resolve_address_size1(const x86_64_val_t *val)
{
    size_t s1;
    size_t s2;
    size_t s3;

    if ( X86_64_VAL_ADDR != val->type ) {
        return 0;
    }

    /* Specified address size */
    s1 = val->u.addr.saddrsize;

    /* Estimated operand size */
    if ( X86_64_ADDR_BASE & val->u.addr.flags ) {
        s2 = regsize(val->u.addr.base);
    } else {
        s2 = 0;
    }
    if ( X86_64_ADDR_OFFSET & val->u.addr.flags ) {
        s3 = regsize(val->u.addr.offset);
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
    instr->imm.sz = enop->imm.sz;
    instr->imm.val = enop->imm.val;

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
_encode_scaled_index(const x86_64_val_t *val, int *idx, int *rexx, int *ss)
{
    int ret;

    assert( X86_64_VAL_ADDR == val->type );

    /* Encode scaled index */
    if ( X86_64_ADDR_OFFSET & val->u.addr.flags ) {
        /* Resolve the register code of the index register */
        ret = _reg_code(val->u.addr.offset, idx, rexx);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }
        if ( 4 == *idx && (REX_NONE == *rexx || REX_FALSE == *rexx) ) {
            /* Invalid index register */
            return -1;
        }

        /* Check scale multiplier */
        if ( X86_64_ADDR_SCALE & val->u.addr.flags ) {
            switch ( val->u.addr.scale ) {
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
_encode_disp(const x86_64_val_t *val, int64_t *disp, size_t *dispsz)
{
    assert( X86_64_VAL_ADDR == val->type );

    if ( val->u.addr.flags & X86_64_ADDR_DISP ) {
        *disp = val->u.addr.disp;
        if ( 0 == *disp ) {
            *dispsz = 0;
        } else if ( *disp >= -128 && *disp <= 127 ) {
            *dispsz = 1;
        } else {
            *dispsz = 4;
        }
    } else {
        *dispsz = 0;
        *disp = 0;
    }

    return 0;
}


/*
 * Encode the second operand with the addr type that specifies a base register
 */
static int
_encode_rm_second_addr_with_base(int reg, int rexr, const x86_64_val_t *val,
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

    assert( X86_64_VAL_ADDR == val->type );
    assert( X86_64_ADDR_BASE & val->u.addr.flags );

    rexx = REX_NONE;
    rexb = REX_NONE;

    /* Is the base register EIP/RIP? */
    if ( REG_EIP == val->u.addr.base || REG_RIP == val->u.addr.base ) {
        /* The base register is EIP/RIP */
        if ( (X86_64_ADDR_OFFSET & val->u.addr.flags)
             || (X86_64_ADDR_SCALE & val->u.addr.flags) ) {
            /* Must not have scaled index */
            return -1;
        }
        mod = 0;
        rm = 5;

        /* Encode ModR/M */
        modrm = _encode_modrm(reg, mod, rm);
        sib = -1;

        /* Encode displacement */
        ret = _encode_disp(val, &disp, &dispsz);
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
        /* No immediate value */
        enop->imm.sz = 0;
        enop->imm.val = 0;

        return 0;
    } else {
        /* The base register is not EIP/RIP and needs to specify SIB */

        /* Resolve the register code of the base register */
        ret = _reg_code(val->u.addr.base, &base, &rexb);
        if ( ret < 0 ) {
            /* Cannot resolve the register code */
            return -1;
        }

        /* Encode displacement */
        ret = _encode_disp(val, &disp, &dispsz);
        if ( ret < 0 ) {
            return -1;
        }

        if ( X86_64_ADDR_OFFSET & val->u.addr.flags ) {
            /* Requires SIB */
            ret = _encode_scaled_index(val, &idx, &rexx, &ss);
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

            if ( 0 == disp ) {
                if ( 5 == base && (REX_NONE == rexb || REX_FALSE == rexb) ) {
                    /* Go to SIB */
                    mod = 1;
                    rm = 4;
                    dispsz = 1;
                    idx = 4;
                    ss = 0;
                    /* Encode ModR/M */
                    modrm = _encode_modrm(reg, mod, rm);
                    sib = _encode_sib(base, idx, ss);
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
            } else if ( 1 == disp ) {
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
            } else if ( 4 == disp ) {
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
        /* No immediate value */
        enop->imm.sz = 0;
        enop->imm.val = 0;

        return 0;
    }

    return 0;
}

/*
 * Encode the second operand with the addr type for the w/o base register case
 */
static int
_encode_rm_second_addr_without_base(int reg, int rexr, const x86_64_val_t *val,
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

    assert( X86_64_VAL_ADDR == val->type );
    assert( !(X86_64_ADDR_BASE & val->u.addr.flags) );

    /* Mod and R/M must be 0 and 4, respectively, for the case w/o base
       register */
    mod = 0;
    rm = 4;
    /* The base register 5 where Mod is 5 means "w/o base register" */
    base = 5;

    ret = _encode_scaled_index(val, &idx, &rexx, &ss);
    if ( ret < 0 ) {
        return -1;
    }

    /* Encode displacement */
    ret = _encode_disp(val, &disp, &dispsz);
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
    /* No immediate value */
    enop->imm.sz = 0;
    enop->imm.val = 0;

    return 0;
}


static int
_encode_rm_second_addr(int reg, int rexr, const x86_64_val_t *val,
                       x86_64_enop_t *enop)
{
    int ret;

    assert( X86_64_VAL_ADDR == val->type );

    if ( X86_64_ADDR_BASE & val->u.addr.flags ) {
        /* With base register */
        ret = _encode_rm_second_addr_with_base(reg, rexr, val, enop);
    } else {
        /* Without base register */
        ret = _encode_rm_second_addr_without_base(reg, rexr, val, enop);
    }

    return 0;
}

static int
_encode_rm_second_reg(int reg, int rexr, const x86_64_val_t *val,
                      x86_64_enop_t *enop)
{
    int ret;
    int mod;
    int rm;
    int rexb;
    int modrm;

    assert( X86_64_VAL_REG == val->type );

    /* Mod = 3 where  R/M is register */
    mod = 3;

    /* Resolve the register code of the second operand */
    ret = _reg_code(val->u.reg, &rm, &rexb);
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
    /* No immediate value */
    enop->imm.sz = 0;
    enop->imm.val = 0;

    return 0;
}


/*
 * Encode operands with R/M
 */
static int
_encode_rm(const x86_64_val_t *val1, const x86_64_val_t *val2,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_VAL_REG != val1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(val1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_VAL_REG == val2->type ) {
        ret = _encode_rm_second_reg(reg, rexr, val2, enop);
        if ( ret < 0 ) {
            return -1;
        }

        return 0;
    } else if ( X86_64_VAL_ADDR == val2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_second_addr(reg, rexr, val2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

static int
_encode_mr(const x86_64_val_t *val1, const x86_64_val_t *val2,
           x86_64_enop_t *enop)
{
    return _encode_rm(val2, val1, enop);
}

static int
_encode_rmi(const x86_64_val_t *val1, const x86_64_val_t *val2,
            const x86_64_val_t *val3, size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexr;

    /* Check the first operand */
    if ( X86_64_VAL_REG != val1->type ) {
        return -1;
    }

    /* Resolve the register code of the first operand */
    ret = _reg_code(val1->u.reg, &reg, &rexr);
    if ( ret < 0 ) {
        /* Cannot resolve the register code */
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_VAL_REG == val2->type ) {
        ret = _encode_rm_second_reg(reg, rexr, val2, enop);
        if ( ret < 0 ) {
            return -1;
        }

        return 0;
    } else if ( X86_64_VAL_ADDR == val2->type ) {
        /* Encode the second operand with the addr type */
        ret = _encode_rm_second_addr(reg, rexr, val2, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    /* Check the third operand */
    if ( X86_64_VAL_IMM != val2->type ) {
        return -1;
    }
    /* Update immediate value */
    enop->imm.sz = immsz;
    enop->imm.val = val3->u.imm;

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
_encode_oi(const x86_64_val_t *val1, const x86_64_val_t *val2, size_t immsz,
           x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_VAL_REG != val1->type ) {
        return -1;
    }
    ret = _reg_code(val1->u.reg, &reg, &rexb);
    if ( ret < 0 ) {
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_VAL_IMM != val2->type ) {
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
    enop->imm.sz = immsz;
    enop->imm.val = val2->u.imm;

    return 0;
}

static int
_encode_mi(const x86_64_val_t *val1, const x86_64_val_t *val2, int reg,
           size_t immsz, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_VAL_REG == val1->type ) {
        ret = _encode_rm_second_reg(reg, rexr, val1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_VAL_ADDR == val1->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_second_addr(reg, rexr, val1, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    /* Check the second operand */
    if ( X86_64_VAL_IMM != val2->type ) {
        return -1;
    }
    /* Update immediate value */
    enop->imm.sz = immsz;
    enop->imm.val = val2->u.imm;

    return 0;
}
static int
_encode_m(const x86_64_val_t *val, int reg, x86_64_enop_t *enop)
{
    int ret;
    int rexr;

    /* /reg */
    rexr = REX_NONE;

    /* Check the first operand */
    if ( X86_64_VAL_REG == val->type ) {
        ret = _encode_rm_second_reg(reg, rexr, val, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else if ( X86_64_VAL_ADDR == val->type ) {
        /* Encode the first operand with the addr type */
        ret = _encode_rm_second_addr(reg, rexr, val, enop);
        if ( ret < 0 ) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}
static int
_encode_o(const x86_64_val_t *val, x86_64_enop_t *enop)
{
    int ret;
    int reg;
    int rexb;

    /* Check the first operand */
    if ( X86_64_VAL_REG != val->type ) {
        return -1;
    }
    ret = _reg_code(val->u.reg, &reg, &rexb);
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
    enop->imm.sz = 0;
    enop->imm.val = 0;

    return 0;
}
static int
_encode_i(const x86_64_val_t *val, size_t immsz, x86_64_enop_t *enop)
{
    /* Check the second operand */
    if ( X86_64_VAL_IMM != val->type ) {
        return -1;
    }
    enop->opreg = -1;
    enop->rex.r = REX_NONE;
    enop->rex.x = REX_NONE;
    enop->rex.b = REX_NONE;
    enop->modrm = -1;
    enop->sib = -1;
    enop->disp.sz = 0;
    enop->disp.val = 0;
    enop->imm.sz = immsz;
    enop->imm.val = val->u.imm;

    return 0;
}






/*
 * Evaluate operands
 */
static int
_eval1(const x86_64_label_table_t *ltbl, x86_64_val_t **val,
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
    *val = x86_64_eval_operand(ltbl, op);
    if ( NULL == *val ) {
        /* Error */
        return -1;
    }

    return 1;
}
static int
_eval2(const x86_64_label_table_t *ltbl, x86_64_val_t **val1,
       x86_64_val_t **val2, const operand_vector_t *operands)
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
    *val1 = x86_64_eval_operand(ltbl, op1);
    if ( NULL == *val1 ) {
        /* Error */
        return -1;
    }
    *val2 = x86_64_eval_operand(ltbl, op2);
    if ( NULL == *val2 ) {
        /* Error */
        free(*val1);
        return -1;
    }

    return 1;
}
static int
_eval3(const x86_64_label_table_t *ltbl, x86_64_val_t **val1,
       x86_64_val_t **val2, x86_64_val_t **val3,
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
    *val1 = x86_64_eval_operand(ltbl, op1);
    if ( NULL == *val1 ) {
        /* Error */
        return -1;
    }
    *val2 = x86_64_eval_operand(ltbl, op2);
    if ( NULL == *val2 ) {
        /* Error */
        free(*val1);
        return -1;
    }
    *val3 = x86_64_eval_operand(ltbl, op3);
    if ( NULL == *val3 ) {
        /* Error */
        free(*val1);
        free(*val2);
        return -1;
    }

    return 1;
}
static int
_eval4(const x86_64_label_table_t *ltbl, x86_64_val_t **val1,
       x86_64_val_t **val2, x86_64_val_t **val3, x86_64_val_t **val4,
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
    *val1 = x86_64_eval_operand(ltbl, op1);
    if ( NULL == *val1 ) {
        /* Error */
        return -1;
    }
    *val2 = x86_64_eval_operand(ltbl, op2);
    if ( NULL == *val2 ) {
        /* Error */
        free(*val1);
        return -1;
    }
    *val3 = x86_64_eval_operand(ltbl, op3);
    if ( NULL == *val3 ) {
        /* Error */
        free(*val1);
        free(*val2);
        return -1;
    }
    *val4 = x86_64_eval_operand(ltbl, op4);
    if ( NULL == *val4 ) {
        /* Error */
        free(*val1);
        free(*val2);
        free(*val3);
        return -1;
    }

    return 1;
}


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
    enop.imm.sz = 0;
    enop.imm.val = 0;
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

static int
_binstr_i(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
          int opc2, int opc3, size_t opsize, const x86_64_val_t *val,
          size_t immsz)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_i(val, immsz, &enop);
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

static int
_binstr_mi(x86_64_instr_t *instr, const x86_64_asm_opt_t *opt, int opc1,
           int opc2, int opc3, int preg, size_t opsize,
           const x86_64_val_t *valm, const x86_64_val_t *vali, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Check pseudo register */
    if ( preg < 0 ) {
        return -EUNKNOWN;
    }

    /* Encode and free the values */
    ret = _encode_mi(valm, vali, preg, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(valm);
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
           int opc2, int opc3, size_t opsize, const x86_64_val_t *valm,
           const x86_64_val_t *valr)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_mr(valm, valr, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(valm);
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
           int opc2, int opc3, size_t opsize, const x86_64_val_t *valr,
           const x86_64_val_t *valm)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_rm(valr, valm, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(valm);
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
            int opc2, int opc3, size_t opsize, const x86_64_val_t *valr,
            const x86_64_val_t *valm, const x86_64_val_t *vali, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_rmi(valr, valm, vali, immsz, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(valm);
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
          int opc2, int opc3, size_t opsize, const x86_64_val_t *valr)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_o(valr, &enop);
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
           int opc2, int opc3, size_t opsize, const x86_64_val_t *valr,
           const x86_64_val_t *vali, size_t immsz)
{
    int ret;
    x86_64_enop_t enop;

    /* Encode and free the values */
    ret = _encode_oi(valr, vali, immsz, &enop);
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
          int opc2, int opc3, int preg, size_t opsize, const x86_64_val_t *valm)
{
    int ret;
    x86_64_enop_t enop;
    ssize_t addrsize;

    /* Encode and free the values */
    ret = _encode_m(valm, preg, &enop);
    if ( ret < 0 ) {
        /* Invalid operand size */
        return -ESIZE;
    }
    /* Obtain address size */
    addrsize = _resolve_address_size1(valm);
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
    x86_64_val_t *val[4];

    /* Obtain the number of operands */
    nr = mvector_size(operands);
    if ( 1 == nr ) {
        /* Evaluate operands */
        ret = _eval1(opt->ltbl, &val[0], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
   } else if ( 2 == nr ) {
        /* Evaluate operands */
        ret = _eval2(opt->ltbl, &val[0], &val[1], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
    } else if ( 3 == nr ) {
        /* Evaluate operands */
        ret = _eval3(opt->ltbl, &val[0], &val[1], &val[2], operands);
        if ( ret < 0 ) {
            return -EOPERAND;
        } else if ( 0 == ret ) {
            return 0;
        }
    } else if ( 3 == nr ) {
        /* Evaluate operands */
        ret = _eval4(opt->ltbl, &val[0], &val[1], &val[2], &val[3], operands);
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
        if ( 2 == nr && _eq_reg(val[0], REG_AL) && _eq_reg(val[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_DX) && _eq_reg(val[1], REG_AL) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_AX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AX) && _eq_reg(val[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_DX) && _eq_reg(val[1], REG_AX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_EAX_DX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_EAX) && _eq_reg(val[1], REG_DX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_NP_DX_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_DX) && _eq_reg(val[1], REG_EAX) ) {
            stat = _binstr_np(instr, opt, opc1, opc2, opc3, opsize);
        }
        break;
    case ENC_I_IMM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_imm16(val[0]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[0],
                             SIZE16);
        }
        break;
    case ENC_I_AL_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AL) && _is_imm8(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, SIZE8, val[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AX) && _is_imm8(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_IMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AX) && _is_imm16(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, SIZE16, val[1],
                             SIZE16);
        }
        break;
    case ENC_I_EAX_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_EAX) && _is_imm8(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[1],
                             SIZE8);
        }
        break;
    case ENC_I_EAX_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_EAX) && _is_imm32(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, SIZE32, val[1],
                             SIZE32);
        }
        break;
    case ENC_I_RAX_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_RAX) && _is_imm32(val[1]) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, SIZE64, val[1],
                             SIZE32);
        }
        break;
    case ENC_I_IMM8_AL:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(val[0]) && _eq_reg(val[1], REG_AL) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[0],
                             SIZE8);
        }
        break;
    case ENC_I_IMM8_AX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(val[0]) && _eq_reg(val[1], REG_AX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[0],
                             SIZE8);
        }
        break;
    case ENC_I_IMM8_EAX:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_imm8(val[0]) && _eq_reg(val[1], REG_EAX) ) {
            stat = _binstr_i(instr, opt, opc1, opc2, opc3, opsize, val[0],
                             SIZE8);
        }
        break;
    case ENC_MI_RM8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr8(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE8,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr16(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE16,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr16(val[0]) && _is_imm16(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE16,
                              val[0], val[1], SIZE16);
        }
        break;
    case ENC_MI_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr32(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE32,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr32(val[0]) && _is_imm32(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE32,
                              val[0], val[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr64(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE64,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr64(val[0]) && _is_imm32(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE64,
                              val[0], val[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_IMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr64(val[0]) && _is_imm64(val[1]) ) {
            stat = _binstr_mi(instr, opt, opc1, opc2, opc3, preg, SIZE64,
                              val[0], val[1], SIZE64);
        }
        break;
    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8_r8(val[0], val[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, SIZE8, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16_r16(val[0], val[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, SIZE16, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32_r32(val[0], val[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, SIZE32, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64_r64(val[0], val[1]) ) {
            stat = _binstr_mr(instr, opt, opc1, opc2, opc3, SIZE64, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE8, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rm16(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE16, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm16(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm32(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE32, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm64(val[0], val[1]) ) {
            stat = _binstr_rm(instr, opt, opc1, opc2, opc3, SIZE64, val[0],
                              val[1]);
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg32(val[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, val[0]);
        }
        break;
    case ENC_O_R64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg64(val[0]) ) {
            stat = _binstr_o(instr, opt, opc1, opc2, opc3, opsize, val[0]);
        }
        break;
    case ENC_OI_R8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_imm8(val[0], val[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  val[0], val[1], SIZE8);
        }
        break;
    case ENC_OI_R16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_imm16(val[0], val[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  val[0], val[1], SIZE16);
        }
        break;
    case ENC_OI_R32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_imm32(val[0], val[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  val[0], val[1], SIZE32);
        }
        break;
    case ENC_OI_R64_IMM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_imm64(val[0], val[1]) ) {
                stat = _binstr_oi(instr, opt, opc1, opc2, opc3, opsize,
                                  val[0], val[1], SIZE64);
        }
        break;
    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_addr8(val[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, SIZE8,
                             val[0]);
        }
        break;
    case ENC_M_RM8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg_addr8(val[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             val[0]);
        }
        break;
    case ENC_M_RM16:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg_addr16(val[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             val[0]);
        }
    case ENC_M_RM32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg_addr32(val[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             val[0]);
        }
        break;
    case ENC_M_RM64:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg_addr64(val[0]) ) {
            stat = _binstr_m(instr, opt, opc1, opc2, opc3, preg, opsize,
                             val[0]);
        }
        break;
    case ENC_RMI_R16_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm8(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE8);
        }
        break;
    case ENC_RMI_R16_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r16_rm16_imm16(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE16);
        }
        break;
    case ENC_RMI_R32_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm8(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE8);
        }
        break;
    case ENC_RMI_R32_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r32_rm32_imm32(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE32);
        }
        break;
    case ENC_RMI_R64_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm8(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE8);
        }
        break;
    case ENC_RMI_R64_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 3 == nr && _is_r64_rm64_imm32(val[0], val[1], val[2]) ) {
            stat = _binstr_rmi(instr, opt, opc1, opc2, opc3, opsize,
                               val[0], val[1], val[2], SIZE32);
        }
        break;
    default:
        stat = 0;
    }

    /* Free values */
    for ( i = 0; i < nr; i++ ) {
        free(val[i]);
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

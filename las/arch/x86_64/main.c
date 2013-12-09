/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "../../las.h"
#include "reg.h"
#include "eval.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>







/* Errors */
#define EUNKNOWN        1
#define EOPERAND        2
#define ESIZE           3






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





#define REXW_NONE       0
#define REXW_YES        1

#define REXR_NONE       0
#define REXR_YES        1

#define REXX_NONE       0
#define REXX_YES        1

#define REXB_NE         0
#define REXB_NONE       0       /* Not encodable */
#define REXB_YES        1


/*
 * Pseudo instruction
 */
struct xxx {
    /* Instruction */
    instr_t *instr;
    /* Offset */
    off_t off;
    /* Estimated size */
    size_t sz;
    /* Max size */
    size_t maxsz;
};

struct y {
    char *label;
    struct xxx *ptr;
};



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

int
_build_instruction(x86_64_target_t target, const x86_64_enop_t *enop,
                   size_t opsize, size_t addrsize, x86_64_instr_t *instr)
{
    int rex;
    int p66 = 0;
    int p67 = 0;
    int rexw = REX_NONE;

    switch ( target ) {
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
    if ( p66 ) {
        instr->prefix3 = 0x66;
    } else {
        instr->prefix3 = -1;
    }
    if ( p67 ) {
        instr->prefix4 = 0x67;
    } else {
        instr->prefix4 = -1;
    }
    instr->rex = rex;
    instr->modrm = enop->modrm;
    instr->sib = enop->sib;
    instr->disp.sz = enop->disp.sz;
    instr->disp.val = enop->disp.val;
    instr->imm.sz = enop->imm.sz;
    instr->imm.val = enop->imm.val;

    return 0;
}
int
_print_instruction(const x86_64_instr_t *instr)
{
    int i;
    int64_t val;

    if ( instr->prefix1 >= 0 ) {
        printf("%.2X", instr->prefix1);
    }
    if ( instr->prefix2 >= 0 ) {
        printf("%.2X", instr->prefix2);
    }
    if ( instr->prefix3 >= 0 ) {
        printf("%.2X", instr->prefix3);
    }
    if ( instr->prefix4 >= 0 ) {
        printf("%.2X", instr->prefix4);
    }
    if ( instr->rex >= 0 ) {
        printf("%.2X", instr->rex);
    }
    if ( instr->opcode1 >= 0 ) {
        printf("%.2X", instr->opcode1);
    }
    if ( instr->opcode2 >= 0 ) {
        printf("%.2X", instr->opcode2);
    }
    if ( instr->opcode3 >= 0 ) {
        printf("%.2X", instr->opcode3);
    }
    if ( instr->modrm >= 0 ) {
        printf("%.2X", instr->modrm);
    }
    if ( instr->sib >= 0 ) {
        printf("%.2X", instr->sib);
    }
    /* Little endian */
    val = instr->disp.val;
    for ( i = 0; i < instr->disp.sz; i++ ) {
        printf("%.2llX", val & 0xff);
        val >>= 8;
    }
    val = instr->imm.val;
    for ( i = 0; i < instr->imm.sz; i++ ) {
        printf("%.2llX", val & 0xff);
        val >>= 8;
    }

    return 0;
}
int
_print_instruction_bin(const x86_64_instr_t *instr)
{
    int i;
    int64_t val;

    if ( instr->prefix1 >= 0 ) {
        printf("%c", instr->prefix1);
    }
    if ( instr->prefix2 >= 0 ) {
        printf("%c", instr->prefix2);
    }
    if ( instr->prefix3 >= 0 ) {
        printf("%c", instr->prefix3);
    }
    if ( instr->prefix4 >= 0 ) {
        printf("%c", instr->prefix4);
    }
    if ( instr->rex >= 0 ) {
        printf("%c", instr->rex);
    }
    if ( instr->opcode1 >= 0 ) {
        printf("%c", instr->opcode1);
    }
    if ( instr->opcode2 >= 0 ) {
        printf("%c", instr->opcode2);
    }
    if ( instr->opcode3 >= 0 ) {
        printf("%c", instr->opcode3);
    }
    if ( instr->modrm >= 0 ) {
        printf("%c", instr->modrm);
    }
    if ( instr->sib >= 0 ) {
        printf("%c", instr->sib);
    }
    /* Little endian */
    val = instr->disp.val;
    for ( i = 0; i < instr->disp.sz; i++ ) {
        printf("%c", (unsigned char)val & 0xff);
        val >>= 8;
    }
    val = instr->imm.val;
    for ( i = 0; i < instr->imm.sz; i++ ) {
        printf("%c", (unsigned char)val & 0xff);
        val >>= 8;
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



typedef enum _x86_64_enc {
    ENC_NP,
    ENC_I_AL_IMM8,
    ENC_I_AX_IMM16,
    ENC_I_EAX_IMM32,
    ENC_I_RAX_IMM32,
    ENC_MI_RM8_IMM8,
    ENC_MI_RM16_IMM8,
    ENC_MI_RM16_IMM16,
    ENC_MI_RM32_IMM8,
    ENC_MI_RM32_IMM32,
    ENC_MI_RM64_IMM8,
    ENC_MI_RM64_IMM32,
    ENC_MR_RM8_R8,
    ENC_MR_RM16_R16,
    ENC_MR_RM32_R32,
    ENC_MR_RM64_R64,
    ENC_RM_R8_RM8,
    ENC_RM_R16_RM16,
    ENC_RM_R32_RM8,
    ENC_RM_R32_RM16,
    ENC_RM_R32_RM32,
    ENC_RM_R64_RM8,
    ENC_RM_R64_RM64,
    ENC_O_R32,
    ENC_O_R64,
    ENC_M_M8,
} x86_64_enc_t;

#define PASS0(ret) do {                         \
    if ( 0 < ret ) { return ret; }              \
    else if ( 0 > ret ) { return 0; }           \
    } while ( 0 )



/*
 * Evaluate operands
 */
static int
_eval1(x86_64_val_t **val, const operand_vector_t *operands)
{
    operand_t *op;

    /* Check the number of operand */
    if ( 1 != mvector_size(operands) ) {
        return 0;
    }
    /* Obtain operands */
    op = mvector_at(operands, 0);
    /* Evaluate operands */
    *val = x86_64_eval_operand(op);
    if ( NULL == *val ) {
        /* Error */
        return -1;
    }

    return 1;
}
static int
_eval2(x86_64_val_t **val1, x86_64_val_t **val2,
       const operand_vector_t *operands)
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
    *val1 = x86_64_eval_operand(op1);
    if ( NULL == *val1 ) {
        /* Error */
        return -1;
    }
    *val2 = x86_64_eval_operand(op2);
    if ( NULL == *val2 ) {
        /* Error */
        free(*val1);
        return -1;
    }

    return 1;
}
static int
_eval3(x86_64_val_t **val1, x86_64_val_t **val2, x86_64_val_t **val3,
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
    *val1 = x86_64_eval_operand(op1);
    if ( NULL == *val1 ) {
        /* Error */
        return -1;
    }
    *val2 = x86_64_eval_operand(op2);
    if ( NULL == *val2 ) {
        /* Error */
        free(*val1);
        return -1;
    }
    *val3 = x86_64_eval_operand(op3);
    if ( NULL == *val3 ) {
        /* Error */
        free(*val1);
        free(*val2);
        return -1;
    }

    return 1;
}


static int
_binstr_np(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
           int opc3, int opsize)
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
    ret = _build_instruction(target, &enop, opsize, 0, instr);
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
_binstr_i(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
          int opc3, size_t opsize, const x86_64_val_t *val, size_t immsz)
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
    ret = _build_instruction(target, &enop, opsize, 0, instr);
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
_binstr_mi(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
           int opc3, int preg, size_t opsize, const x86_64_val_t *valm,
           const x86_64_val_t *vali, size_t immsz)
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
    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
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
_binstr_mr(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
           int opc3, size_t opsize, const x86_64_val_t *valm,
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
    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
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
_binstr_rm(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
           int opc3, size_t opsize, const x86_64_val_t *valr,
           const x86_64_val_t *valm)
{
    return _binstr_rm(instr, target, opc1, opc2, opc3, opsize, valm, valr);
}

static int
_binstr_o(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
          int opc3, size_t opsize, const x86_64_val_t *valr)
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
    ret = _build_instruction(target, &enop, opsize, 0, instr);
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
_binstr_m(x86_64_instr_t *instr, x86_64_target_t target, int opc1, int opc2,
          int opc3, int preg, size_t opsize, const x86_64_val_t *valm)
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
    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
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
binstr(x86_64_instr_t *instr, x86_64_target_t target, int opsize, int opc1,
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
       ret = _eval1(&val[0], operands);
       if ( ret < 0 ) {
           return -EOPERAND;
       } else if ( 0 == ret ) {
           return 0;
       }
   } else if ( 2 == nr ) {
       /* Evaluate operands */
       ret = _eval2(&val[0], &val[1], operands);
       if ( ret < 0 ) {
           return -EOPERAND;
       } else if ( 0 == ret ) {
           return 0;
       }
   } else if ( 3 == nr ) {
       /* Evaluate operands */
       ret = _eval3(&val[0], &val[1], &val[2], operands);
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
            stat = _binstr_np(instr, target, opc1, opc2, opc3, opsize);
        }
    case ENC_I_AL_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AL) && _is_imm8(val[1]) ) {
            stat = _binstr_i(instr, target, opc1, opc2, opc3, SIZE8, val[1],
                             SIZE8);
        }
        break;
    case ENC_I_AX_IMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_AX) && _is_imm16(val[1]) ) {
            stat = _binstr_i(instr, target, opc1, opc2, opc3, SIZE16, val[1],
                             SIZE16);
        }
        break;
    case ENC_I_EAX_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_EAX) && _is_imm32(val[1]) ) {
            stat = _binstr_i(instr, target, opc1, opc2, opc3, SIZE32, val[1],
                             SIZE32);
        }
        break;
    case ENC_I_RAX_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _eq_reg(val[0], REG_RAX) && _is_imm32(val[1]) ) {
            stat = _binstr_i(instr, target, opc1, opc2, opc3, SIZE64, val[1],
                             SIZE32);
        }
        break;
    case ENC_MI_RM8_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr8(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE8,
                              val[0], val[1], SIZE8);
        }
    case ENC_MI_RM16_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr16(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE16,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM16_IMM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr16(val[0]) && _is_imm16(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE16,
                              val[0], val[1], SIZE16);
        }
        break;
    case ENC_MI_RM32_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr32(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE32,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM32_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr32(val[0]) && _is_imm32(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE32,
                              val[0], val[1], SIZE32);
        }
        break;
    case ENC_MI_RM64_IMM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr64(val[0]) && _is_imm8(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE64,
                              val[0], val[1], SIZE8);
        }
        break;
    case ENC_MI_RM64_IMM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_reg_addr64(val[0]) && _is_imm32(val[1]) ) {
            stat = _binstr_mi(instr, target, opc1, opc2, opc3, preg, SIZE64,
                              val[0], val[1], SIZE32);
        }
        break;
    case ENC_MR_RM8_R8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm8_r8(val[0], val[1]) ) {
            stat = _binstr_mr(instr, target, opc1, opc2, opc3, SIZE8, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM16_R16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm16_r16(val[0], val[1]) ) {
            stat = _binstr_mr(instr, target, opc1, opc2, opc3, SIZE16, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM32_R32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm32_r32(val[0], val[1]) ) {
            stat = _binstr_mr(instr, target, opc1, opc2, opc3, SIZE32, val[0],
                              val[1]);
        }
        break;
    case ENC_MR_RM64_R64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_rm64_r64(val[0], val[1]) ) {
            stat = _binstr_mr(instr, target, opc1, opc2, opc3, SIZE64, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R8_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r8_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, SIZE8, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R16_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r16_rm16(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, SIZE16, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM16:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm16(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R32_RM32:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r32_rm32(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, SIZE32, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R64_RM8:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm8(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, opsize, val[0],
                              val[1]);
        }
        break;
    case ENC_RM_R64_RM64:
        /* Check the number of operands and format */
        if ( 2 == nr && _is_r64_rm64(val[0], val[1]) ) {
            stat = _binstr_rm(instr, target, opc1, opc2, opc3, SIZE64, val[0],
                              val[1]);
        }
        break;
    case ENC_O_R32:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_reg32(val[0]) ) {
            stat = _binstr_o(instr, target, opc1, opc2, opc3, SIZE32, val[0]);
        }
        break;
    case ENC_M_M8:
        /* Check the number of operands and format */
        if ( 1 == nr && _is_addr8(val[0]) ) {
            stat = _binstr_m(instr, target, opc1, opc2, opc3, preg, SIZE8,
                             val[0]);
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
 * ADD (Vol. 2A 3-27)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      04 ib           ADD AL,imm8             I       Valid   Valid
 *      05 iw           ADD AX,imm16            I       Valid   Valid
 *      05 ib           ADD EAX,imm32           I       Valid   Valid
 *      REX.W + 05 id   ADD RAX,imm32           I       Valid   N.E.
 *      80 /0 ib        ADD r/m8,imm8           MI      Valid   Valid
 *      REX + 80 /0 ib  ADD r/m8*,imm8          MI      Valid   N.E.
 *      81 /0 iw        ADD r/m16,imm16         MI      Valid   Valid
 *      81 /0 id        ADD r/m32,imm32         MI      Valid   Valid
 *      REX.W + 81 /0 id
 *                      ADD r/m64,imm32         MI      Valid   N.E.
 *      83 /0 ib        ADD r/m16,imm8          MI      Valid   Valid
 *      83 /0 ib        ADD r/m32,imm8          MI      Valid   Valid
 *      REX.W + 83 /0 ib
 *                      ADD r/m64,imm8          MI      Valid   N.E.
 *      00 /r           ADD r/m8,r8             MR      Valid   Valid
 *      REX + 00 /r     ADD r/m8*,r8*           NR      Valid   N.E.
 *      01 /r           ADD r/m16,r16           MR      Valid   Valid
 *      01 /r           ADD r/m32,r32           MR      Valid   Valid
 *      REX.W + 01 /r   ADD r/m64,r64           MR      Valid   N.E.
 *      02 /r           ADD r8,r/m8             RM      Valid   Valid
 *      REX + 02 /r     ADD r8*,r/m8*           RM      Valid   Valid
 *      03 /r           ADD r16,r/m16           RM      Valid   Valid
 *      03 /r           ADD r32,r/m32           RM      Valid   Valid
 *      REX.W + 03 /r   ADD r64,r/m64           RM      Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r,w)  imm8/16/32      NA              NA
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 */
static int
_add(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x04, -1, -1, -1, ops, ENC_I_AL_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x05, -1, -1, -1, ops, ENC_I_AX_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x05, -1, -1, -1, ops, ENC_I_EAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x05, -1, -1, -1, ops, ENC_I_RAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x80, -1, -1, 0, ops, ENC_MI_RM8_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 0, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 0, ops, ENC_MI_RM16_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 0, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 0, ops, ENC_MI_RM32_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 0, ops, ENC_MI_RM64_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 0, ops, ENC_MI_RM64_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x00, -1, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, -1, 0x01, -1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x01, -1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x01, -1, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x02, -1, -1, -1, ops, ENC_RM_R8_RM8));
    PASS0(binstr(instr, tgt, -1, 0x03, -1, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, -1, 0x03, -1, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, -1, 0x03, -1, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * AND (Vol. 2A 3-51)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      24 ib           AND AL,imm8             I       Valid   Valid
 *      25 iw           AND AX,imm16            I       Valid   Valid
 *      25 id           AND EAX,imm32           I       Valid   Valid
 *      REX.W + 25 id   AND RAX,imm32           I       Valid   N.E.
 *      80 /4 ib        AND r/m8,imm8           MI      Valid   Valid
 *      REX + 80 /4 ib  AND r/m8*,imm8          MI      Valid   N.E.
 *      81 /4 iw        AND r/m16,imm16         MI      Valid   Valid
 *      81 /4 id        AND r/m32,imm32         MI      Valid   Valid
 *      REX.W + 81 /4 id
 *                      AND r/m64,imm32         MI      Valid   N.E.
 *      83 /4 ib        AND r/m16,imm8          MI      Valid   Valid
 *      83 /4 ib        AND r/m32,imm8          MI      Valid   Valid
 *      REX.W + 83 /4 ib
 *                      AND r/m64,imm8          MI      Valid   Valid
 *      20 /r           AND r/m8,r8             MR      Valid   Valid
 *      REX + 20 /r     AND r/m8*,r8*           MR      Valid   N.E.
 *      21 /r           AND r/m16,r16           MR      Valid   Valid
 *      21 /r           AND r/m32,r32           MR      Valid   Valid
 *      REX.W + 21 /r   AND r/m64,r64           MR      Valid   N.E.
 *      22 /r           AND r8,r/m8             RM      Valid   Valid
 *      REX + 22 /r     AND r8*,r/m8*           RM      Valid   N.E.
 *      23 /r           AND r16,r/m16           RM      Valid   Valid
 *      23 /r           AND r32,r/m32           RM      Valid   Valid
 *      REX.W + 23 /r   AND r64,r/m64           RM      Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r,w)  imm8/16/32      NA              NA
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 */
static int
_and(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x24, -1, -1, -1, ops, ENC_I_AL_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x25, -1, -1, -1, ops, ENC_I_AX_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x25, -1, -1, -1, ops, ENC_I_EAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x25, -1, -1, -1, ops, ENC_I_RAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x80, -1, -1, 4, ops, ENC_MI_RM8_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 4, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 4, ops, ENC_MI_RM16_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 4, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 4, ops, ENC_MI_RM32_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 4, ops, ENC_MI_RM64_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 4, ops, ENC_MI_RM64_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x20, -1, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, -1, 0x21, -1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x21, -1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x21, -1, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x22, -1, -1, -1, ops, ENC_RM_R8_RM8));
    PASS0(binstr(instr, tgt, -1, 0x23, -1, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, -1, 0x23, -1, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, -1, 0x23, -1, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * BSF (Vol. 2A 3-74)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F BC /r        BSF r16,r/m16           RM      Valid   Valid
 *      0F BC /r        BSF r32,r/m32           RM      Valid   Valid
 *      REX.W + 0F BC /r
 *                      BSF r64,r/m64           RM      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(w)    ModRM:r/m(r)    NA              NA
 */
static int
_bsf(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbc, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbc, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbc, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * BSR (Vol. 2A 3-76)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F BD /r        BSR r16,r/m16           RM      Valid   Valid
 *      0F BD /r        BSR r32,r/m32           RM      Valid   Valid
 *      REX.W + 0F BD /r
 *                      BSR r64,r/m64           RM      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(w)    ModRM:r/m(r)    NA              NA
 */
static int
_bsr(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbd, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbd, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbd, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * BSWAP (Vol. 2A 3-78)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F C8+rd        BSWAP r32               O       Valid   Valid
 *      REX.W + 0F C8+rd
 *                      BSWAP r64               O       Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      O       opcode+rd(r,w)  NA              NA              NA
 */
static int
_bswap(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xc8, -1, -1, ops, ENC_O_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xc8, -1, -1, ops, ENC_O_R64));

    return -EOPERAND;
}

/*
 * BT (Vol. 2A 3-79)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F A3 /r        BT r/m16,r16            MR      Valid   Valid
 *      0F A3 /r        BT r/m32,r32            MR      Valid   Valid
 *      REX.W + 0F A3 /r
 *                      BT r/m64,r64            MR      Valid   N.E.
 *      0F BA /4 ib     BT r/m16,imm8           MI      Valid   Valid
 *      0F BA /4 ib     BT r/m32,imm8           MI      Valid   Valid
 *      REX.W + 0F BA /4 ib
 *                      BT r/m64,imm8           MI      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(r)    ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r)    imm8            NA              NA
 */
static int
_bt(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xa3, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xa3, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xa3, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 4, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 4, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 4, ops, ENC_MI_RM64_IMM8));

    return -EOPERAND;
}


/*
 * BTC (Vol. 2A 3-81)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F BB /r        BTC r/m16,r16           MR      Valid   Valid
 *      0F BB /r        BTC r/m32,r32           MR      Valid   Valid
 *      REX.W + 0F BB /r
 *                      BTC r/m64,r64           MR      Valid   N.E.
 *      0F BA /7 ib     BTC r/m16,imm8          MI      Valid   Valid
 *      0F BA /7 ib     BTC r/m32,imm8          MI      Valid   Valid
 *      REX.W + 0F BA /7 ib
 *                      BTC r/m64,imm8          MI      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(r)    ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r)    imm8            NA              NA
 */
int
_btc(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbb, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbb, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xbb, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 7, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 7, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 7, ops, ENC_MI_RM64_IMM8));

    return -EOPERAND;
}

/*
 * BTR (Vol. 2A 3-83)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F B3 /r        BTR r/m16,r16           MR      Valid   Valid
 *      0F B3 /r        BTR r/m32,r32           MR      Valid   Valid
 *      REX.W + 0F B3 /r
 *                      BTR r/m64,r64           MR      Valid   N.E.
 *      0F BA /6 ib     BTR r/m16,imm8          MI      Valid   Valid
 *      0F BA /6 ib     BTR r/m32,imm8          MI      Valid   Valid
 *      REX.W + 0F BA /6 ib
 *                      BTR r/m64,imm8          MI      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(r)    ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r)    imm8            NA              NA
 */
int
_btr(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb3, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb3, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb3, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 6, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 6, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 6, ops, ENC_MI_RM64_IMM8));

    return -EOPERAND;
}

/*
 * BTS (Vol. 2A 3-85)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F AB /r        BTS r/m16,r16           MR      Valid   Valid
 *      0F AB /r        BTS r/m32,r32           MR      Valid   Valid
 *      REX.W + 0F AB /r
 *                      BTS r/m64,r64           MR      Valid   N.E.
 *      0F BA /5 ib     BTS r/m16,imm8          MI      Valid   Valid
 *      0F BA /5 ib     BTS r/m32,imm8          MI      Valid   Valid
 *      REX.W + 0F BA /5 ib
 *                      BTS r/m64,imm8          MI      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(r)    ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r)    imm8            NA              NA
 */
int
_bts(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xab, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xab, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xab, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 5, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 5, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xba, -1, 5, ops, ENC_MI_RM64_IMM8));

    return -EOPERAND;
}

/*
 * CBW/CWDE/CDQE (Vol. 2A 3-100)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      98              CBW                     NP      Valid   Valid
 *      98              CWDE                    NP      Valid   Valid
 *      REX.W + 98      CDQE                    NP      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_cbw(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE16, 0x98, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
int
_cwde(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE32, 0x98, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
int
_cdqe(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE64, 0x98, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}

/*
 * CLC (Vol. 2A 3-101)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F8              CLC                     NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_clc(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0xf8, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}

/*
 * CLD (Vol. 2A 3-102)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FC              CLD                     NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_cld(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0xfc, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}

/*
 * CLFLUSH (Vol. 2A 3-103)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F AE /7        CLFLUSH m8              M       Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M(w)       NA              NA              NA
 */
int
_clflush(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xae, -1, 7, ops, ENC_M_M8));

    return -EOPERAND;
}

/*
 * CLI (Vol. 2A 3-105)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FA              CLI                     NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_cli(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0xfa, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}

/*
 * CMP (Vol. 2A 3-113)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      3C ib           CMP AL,imm8             I       Valid   Valid
 *      3D iw           CMP AX,imm16            I       Valid   Valid
 *      3D id           CMP EAX,imm32           I       Valid   Valid
 *      REX.W + 3D id   CMP RAX,imm32           I       Valid   N.E.
 *      80 /7 ib        CMP r/m8,imm8           MI      Valid   Valid
 *      REX + 80 /7 ib  CMP r/m8*,imm8          MI      Valid   N.E.
 *      81 /7 iw        CMP r/m16,imm16         MI      Valid   Valid
 *      81 /7 id        CMP r/m32,imm32         MI      Valid   Valid
 *      REX.W + 81 /7 id
 *                      CMP r/m64,imm32         MI      Valid   N.E.
 *      83 /7 ib        CMP r/m16,imm8          MI      Valid   Valid
 *      83 /7 ib        CMP r/m32,imm8          MI      Valid   Valid
 *      REX.W + 83 /7 ib
 *                      CMP r/m64,imm8          MI      Valid   Valid
 *      38 /r           CMP r/m8,r8             MR      Valid   Valid
 *      REX + 38 /r     CMP r/m8*,r8*           MR      Valid   N.E.
 *      39 /r           CMP r/m16,r16           MR      Valid   Valid
 *      39 /r           CMP r/m32,r32           MR      Valid   Valid
 *      REX.W + 39 /r   CMP r/m64,r64           MR      Valid   N.E.
 *      3A /r           CMP r8,r/m8             RM      Valid   Valid
 *      REX + 3A /r     CMP r8*,r/m8*           RM      Valid   N.E.
 *      3B /r           CMP r16,r/m16           RM      Valid   Valid
 *      3B /r           CMP r32,r/m32           RM      Valid   Valid
 *      REX.W + 3B /r   CMP r64,r/m64           RM      Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r,w)  imm8/16/32      NA              NA
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 */
static int
_cmp(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x3c, -1, -1, -1, ops, ENC_I_AL_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x3d, -1, -1, -1, ops, ENC_I_AX_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x3d, -1, -1, -1, ops, ENC_I_EAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x3d, -1, -1, -1, ops, ENC_I_RAX_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x80, -1, -1, 7, ops, ENC_MI_RM8_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 7, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 7, ops, ENC_MI_RM16_IMM16));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 7, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 7, ops, ENC_MI_RM32_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x83, -1, -1, 7, ops, ENC_MI_RM64_IMM8));
    PASS0(binstr(instr, tgt, -1, 0x81, -1, -1, 7, ops, ENC_MI_RM64_IMM32));
    PASS0(binstr(instr, tgt, -1, 0x38, -1, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, -1, 0x39, -1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x39, -1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x39, -1, -1, -1, ops, ENC_MR_RM64_R64));
    PASS0(binstr(instr, tgt, -1, 0x3a, -1, -1, -1, ops, ENC_RM_R8_RM8));
    PASS0(binstr(instr, tgt, -1, 0x3b, -1, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, -1, 0x3b, -1, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, -1, 0x3b, -1, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * CMPXCHG (Vol. 2A 3-140)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F B0 /r        CMPXCHG r/m8,r8         MR      Valid   Valid*
 *      REX + 0F B0 /r  CMPXCHG r/m8**,r8       MR      Valid   N.E.
 *      0F B1 /r        CMPXCHG r/m16,r16       MR      Valid   Valid*
 *      0F B1 /r        CMPXCHG r/m32,r32       MR      Valid   Valid*
 *      REX.W + 0F B1 /r
 *                      CMPXCHG r/m64,r64       MR      Valid   N.E.
 *
 *      *  See IA-32 Architecture Compatibility section in the manual
 *      ** In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *         used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 */
static int
_cmpxchg(x86_64_target_t tgt, const operand_vector_t *ops,
         x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb0, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xb1, -1, -1, ops, ENC_MR_RM64_R64));

    return -EOPERAND;
}

/*
 * CPUID (Vol. 2A 3-148)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F A2           CPUID                   NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_cpuid(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, -1, 0x0f, 0xa2, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}

/*
 * CRC32 (Vol. 2A 3-176)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F2 0F 38 F0 /r  CRC32 r32,r/m8          RM      Valid   Valid
 *      F2 REX 0F 38 F0 /r
 *                      CRC32 r32,r/m8*         RM      Valid   Valid
 *      F2 0F 38 F1 /r  CRC32 r32,r/m16         RM      Valid   Valid
 *      F2 0F 38 F1 /r  CRC32 r32,r/m32         RM      Valid   Valid
 *      F2 REX.W 0F 38 F0 /r
 *                      CRC32 r64,r/m8          RM      Valid   Valid
 *      F2 REX.W 0F 38 F1 /r
 *                      CRC32 r64,r/m64         RM      Valid   Valid
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 */
static int
_crc32(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    /* FIXME */
    instr->prefix1 = 0xf2;

    PASS0(binstr(instr, tgt, SIZE8, 0x0f, 0x38, 0xf0, -1, ops, ENC_RM_R32_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x0f, 0x38, 0xf1, -1, ops,
                 ENC_RM_R32_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x0f, 0x38, 0xf1, -1, ops,
                 ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x0f, 0x38, 0xf0, -1, ops,
                 ENC_RM_R64_RM8));
    PASS0(binstr(instr, tgt, SIZE64, 0x0f, 0x38, 0xf1, -1, ops,
                 ENC_RM_R64_RM64));

    return -EOPERAND;

}

/*
 * CWD/CDQ/CQO (Vol. 2A 3-216)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      99              CWD                     NP      Valid   Valid
 *      99              CDQ                     NP      Valid   Valid
 *      REX.W + 99      CQO                     NP      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_cwd(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = SIZE16;
    addrsize = 0;
    opcode1 = 0x99;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}
int
_cdq(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = SIZE32;
    addrsize = 0;
    opcode1 = 0x99;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}
int
_cqo(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = SIZE64;
    addrsize = 0;
    opcode1 = 0x99;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}

/*
 * DAA (Vol. 2A 3-217)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      27              DAA                     NP      Invalid Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_daa(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = 0;
    addrsize = 0;
    opcode1 = 0x27;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}

/*
 * DAS (Vol. 2A 3-219)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      2F              DAS                     NP      Invalid Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_das(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = 0;
    addrsize = 0;
    opcode1 = 0x2f;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}

/*
 * DEC (Vol. 2A 3-221)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FE /1           DEC r/m8                M       Valid   Valid
 *      REX + FE /1     DEC r/m8*               M       Valid   N.E.
 *      FF /1           DEC r/m16               M       Valid   Valid
 *      FF /1           DEC r/m32               M       Valid   Valid
 *      REX.W + FF /1   DEC r/m64               M       Valid   N.E.
 *      48 +rw          DEC r16                 O       N.E.    Valid
 *      48 +rd          DEC r32                 O       N.E.    Valid
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M:r/m(r,w) NA              NA              NA
 *      O       opcode+rd(r,w)  NA              NA              NA
 */
static int
_dec(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    operand_t *op;
    x86_64_val_t *val;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 1 == mvector_size(operands) ) {
        op = mvector_at(operands, 0);

        val = x86_64_eval_operand(op);
        if ( NULL == val ) {
            /* Error */
            return -1;
        }

        if ( _is_reg_addr8(val) ) {
            ret = _encode_m(val, 1, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xfe;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val) ) {
            ret = _encode_m(val, 1, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val) ) {
            ret = _encode_m(val, 1, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val) ) {
            ret = _encode_m(val, 1, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * DIV (Vol. 2A 3-223)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F6 /6           DIV r/m8                M       Valid   Valid
 *      REX + F6 /6     DIV r/m8*               M       Valid   N.E.
 *      F7 /6           DIV r/m16               M       Valid   Valid
 *      F7 /6           DIV r/m32               M       Valid   Valid
 *      REX.W + F7 /6   DIV r/m64               M       Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M:r/m(r,w) NA              NA              NA
 */
static int
_div(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    operand_t *op;
    x86_64_val_t *val;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 1 == mvector_size(operands) ) {
        op = mvector_at(operands, 0);

        val = x86_64_eval_operand(op);
        if ( NULL == val ) {
            /* Error */
            return -1;
        }

        if ( _is_reg_addr8(val) ) {
            ret = _encode_m(val, 6, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf6;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val) ) {
            ret = _encode_m(val, 6, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val) ) {
            ret = _encode_m(val, 6, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val) ) {
            ret = _encode_m(val, 6, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * HLT (Vol. 2A 3-368)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F4              HLT                     NP      Valid  Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_hlt(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = 0;
    addrsize = 0;
    opcode1 = 0xf4;
    opcode2 = -1;
    opcode3 = -1;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}

/*
 * IDIV (Vol. 2A 3-375)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F6 /7           IDIV r/m8               M       Valid   Valid
 *      REX + F6 /7     IDIV r/m8*              M       Valid   N.E.
 *      F7 /7           IDIV r/m16              M       Valid   Valid
 *      F7 /7           IDIV r/m32              M       Valid   Valid
 *      REX.W + F7 /7   IDIV r/m64              M       Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M:r/m(r,w) NA              NA              NA
 */
static int
_idiv(x86_64_target_t target, const operand_vector_t *operands,
      x86_64_instr_t *instr)
{
    operand_t *op;
    x86_64_val_t *val;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 1 == mvector_size(operands) ) {
        op = mvector_at(operands, 0);

        val = x86_64_eval_operand(op);
        if ( NULL == val ) {
            /* Error */
            return -1;
        }

        if ( _is_reg_addr8(val) ) {
            ret = _encode_m(val, 7, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf6;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val) ) {
            ret = _encode_m(val, 7, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val) ) {
            ret = _encode_m(val, 7, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val) ) {
            ret = _encode_m(val, 7, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * IMUL (Vol. 2A 3-378)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F6 /5           IMUL r/m8*              M       Valid   Valid
 *      F7 /5           IMUL r/m16              M       Valid   Valid
 *      F7 /5           IMUL r/m32              M       Valid   Valid
 *      REX.W + F7 /5   IMUL r/m64              M       Valid   N.E.
 *      0F AF /r        IMUL r16,r/m16          RM      Valid   Valid
 *      0F AF /r        IMUL r32,r/m32          RM      Valid   Valid
 *      REX.W + 0F AF /r
 *                      IMUL r64,r/m64          RM      Valid   N.E.
 *      6B /r ib        IMUL r16,r/m16,imm8     RMI     Valid   Valid
 *      6B /r ib        IMUL r32,r/m32,imm8     RMI     Valid   Valid
 *      REX.W + 6B /r ib
 *                      IMUL r64,r/m64,imm8     RMI     Valid   N.E.
 *      69 /r iw        IMUL r16,r/m16,imm16    RMI     Valid   Valid
 *      69 /r id        IMUL r32,r/m32,imm32    RMI     Valid   Valid
 *      REX.W + 69 /r id
 *                      IMUL r64,r/m64,imm32    RMI     Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M:r/m(r,w) NA              NA              NA
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 *      RMI     ModRM:reg(r,w)  ModRM:r/m(r)    imm8/16/32      NA
 */
static int
_imul(x86_64_target_t target, const operand_vector_t *operands,
      x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    operand_t *op3;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    x86_64_val_t *val3;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 1 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }

        if ( _is_reg_addr8(val1) ) {
            ret = _encode_m(val1, 5, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xf6;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val1) ) {
            ret = _encode_m(val1, 5, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val1) ) {
            ret = _encode_m(val1, 5, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val1) ) {
            ret = _encode_m(val1, 5, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xf7;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);

        return 0;
    } else if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _is_r16_rm16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x0f;
            opcode2 = 0xaf;
            opcode3 = -1;
        } else if ( _is_r32_rm32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x0f;
            opcode2 = 0xaf;
            opcode3 = -1;
        } else if ( _is_r64_rm64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x0f;
            opcode2 = 0xaf;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else if ( 3 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);
        op3 = mvector_at(operands, 2);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }
        val3 = x86_64_eval_operand(op3);
        if ( NULL == val3 ) {
            /* Error */
            free(val1);
            free(val2);
            return -1;
        }

        if ( _is_r16_rm16_imm8(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE8, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x6b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r32_rm32_imm8(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE8, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x6b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r64_rm64_imm8(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE8, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x6b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r16_rm16_imm16(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE16, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x69;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r32_rm32_imm32(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE32, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x69;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r64_rm64_imm32(val1, val2, val3) ) {
            ret = _encode_rmi(val1, val2, val3, SIZE32, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x69;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            free(val3);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            free(val3);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);
        free(val3);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * IN (Vol. 2A 3-382)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      E4 ib           IN AL,imm8              I       Valid   Valid
 *      E5 ib           IN AX,imm8              I       Valid   Valid
 *      E5 ib           IN EAX,imm8             I       Valid   Valid
 *      EC              IN AL,DX                NP      Valid   Valid
 *      ED              IN AX,DX                NP      Valid   Valid
 *      ED              IN EAX,DX               NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      I       imm8            NA              NA              NA
 *      NP      NA              NA              NA              NA
 */
static int
_in(x86_64_target_t target, const operand_vector_t *operands,
    x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _eq_reg(val1, REG_AL) && _is_imm8(val2) ) {
            ret = _encode_i(val2, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe4;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_AX) && _is_imm8(val2) ) {
            ret = _encode_i(val2, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe5;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_EAX) && _is_imm8(val2) ) {
            ret = _encode_i(val2, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe5;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_AL) && _eq_reg(val2, REG_DX) ) {
            ret = 0;
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xec;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_AX) && _eq_reg(val2, REG_DX) ) {
            ret = 0;
            opsize = SIZE16;
            addrsize = 0;
            opcode1 = 0xed;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_EAX) && _eq_reg(val2, REG_DX) ) {
            ret = 0;
            opsize = SIZE32;
            addrsize = 0;
            opcode1 = 0xed;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * INC (Vol. 2A 3-384)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FE /0           INC r/m8                M       Valid   Valid
 *      REX + FE /0     INC r/m8*               M       Valid   N.E.
 *      FF /0           INC r/m16               M       Valid   Valid
 *      FF /0           INC r/m32               M       Valid   Valid
 *      REX.W + FF /0   INC r/m64               M       Valid   N.E.
 *      40 +rw          INC r16                 O       N.E.    Valid
 *      40 +rd          INC r32                 O       N.E.    Valid
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed if a REX prefix is
 *        used
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M:r/m(r,w) NA              NA              NA
 *      O       opcode+rd(r,w)  NA              NA              NA
 */
static int
_inc(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    operand_t *op;
    x86_64_val_t *val;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 1 == mvector_size(operands) ) {
        op = mvector_at(operands, 0);

        val = x86_64_eval_operand(op);
        if ( NULL == val ) {
            /* Error */
            return -1;
        }

        if ( _is_reg_addr8(val) ) {
            ret = _encode_m(val, 0, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xfe;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val) ) {
            ret = _encode_m(val, 0, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val) ) {
            ret = _encode_m(val, 0, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val) ) {
            ret = _encode_m(val, 0, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val);
            opcode1 = 0xff;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * JMP (Vol. 2A 3-424)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      EB cb           JMP rel8                D       Valid   Valid
 *      E9 cw           JMP rel16               D       N.S.    Valid
 *      E9 cd           JMP rel32               D       Valid   Valid
 *      FF /4           JMP r/m16               M       N.S.    Valid
 *      FF /4           JMP r/m32               M       N.S.    Valid
 *      FF /4           JMP r/m64               M       Valid   N.E.
 *      EA cd           JMP ptr16:16            D       Inv.    Valid
 *      EA cp           JMP ptr16:32            D       Inv.    Valid
 *      FF /5           JMP m16:16              D       Valid   Valid
 *      FF /5           JMP m16:32              D       Valid   Valid
 *      REX.W + FF /5   JMP m16:64              D       Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      D       Offset          NA              NA              NA
 *      M       ModR/M(r)       NA              NA              NA
 */
int
_jmp(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    if ( 1 != mvector_size(operands) ) {
        return -EOPERAND;
    }
    /* To be implemented */

    return -EUNKNOWN;
}

/*
 * MONITOR (Vol. 2B 4-27)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F 01 C8        MONITOR                 NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
int
_monitor(x86_64_target_t target, const operand_vector_t *operands,
         x86_64_instr_t *instr)
{
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 0 != mvector_size(operands) ) {
        return -EOPERAND;
    }

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
    opsize = 0;
    addrsize = 0;
    opcode1 = 0x0f;
    opcode2 = 0x01;
    opcode3 = 0xc8;

    ret = _build_instruction(target, &enop, opsize, addrsize, instr);
    if ( ret < 0 ) {
        return -EOPERAND;
    }
    instr->opcode1 = opcode1;
    instr->opcode2 = opcode2;
    instr->opcode3 = opcode3;

    return 0;
}


/*
 * MOV (Vol. 2B 4-29)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      88 /r           MOV r/m8,r8             MR      Valid   Valid
 *      REX + 88 /r     MOV r/m8***,r8***       MR      Valid   N.E.
 *      89 /r           MOV r/m16,r16           MR      Valid   Valid
 *      89 /r           MOV r/m32,r32           MR      Valid   Valid
 *      REX.W + 89 /r   MOV r/m32,r32           MR      Valid   N.E.
 *      8A /r           MOV r8,r/m8             RM      Valid   Valid
 *      REX + 8A /r     MOV r8***,r/m8***       RM      Valid   N.E.
 *      8B /r           MOV r16,r/m16           RM      Valid   Valid
 *      8B /r           MOV r32,r/m32           RM      Valid   Valid
 *      REX.W + 8B /r   MOV r64,r/m64           RM      Valid   N.E.
 *      8C /r           MOV r/m16,Sreg**        MR      Valid   Valid
 *      REX.W 8C /r     MOV r/m64,Sreg**        MR      Valid   Valid
 *      8E /r           MOV Sreg,r/m16**        RM      Valid   Valid
 *      REX.W 8E /r     MOV Sreg,r/m16**        RM      Valid   Valid
 *      A0              MOV AL,moffs8*          FD      Valid   Valid
 *      REX.W + A0      MOV AL,moffs8*          FD      Valid   N.E.
 *      A1              MOV AX,moffs16*         FD      Valid   Valid
 *      A1              MOV EAX,moffs32*        FD      Valid   Valid
 *      REX.W + A1      MOV RAX,moffs64*        FD      Valid   N.E.
 *      A2              MOV moffs8,AL           TD      Valid   Valid
 *      REX.W + A2      MOV moffs8***,AL        TD      Valid   N.E.
 *      A3              MOV moffs16*,AX         TD      Valid   Valid
 *      A3              MOV moffs32*,EAX        TD      Valid   Valid
 *      REX.W + A3      MOV moffs64*,RAX        TD      Valid   N.E.
 *      B0+ rb ib       MOV r8,imm8             OI      Valid   Valid
 *      REX + B0+ rb ib MOV r8***,imm8          OI      Valid   N.E.
 *      B8+ rw iw       MOV r16,imm16           OI      Valid   Valid
 *      B8+ rd id       MOV r32,imm32           OI      Valid   Valid
 *      REX.W + B8+ rd io
 *                      MOV r64,imm64           OI      Valid   N.E.
 *      C6 /0 ib        MOV r/m8,imm8           MI      Valid   Valid
 *      REX + C6 /0 ib  MOV r/m8***,imm8        MI      Valid   N.E.
 *      C7 /0 iw        MOV r/m16,imm16         MI      Valid   Valid
 *      C7 /0 id        MOV r/m32,imm32         MI      Valid   Valid
 *      REX.W + C7 /0 io
 *                      MOV r/m64,imm64         MI      Valid   N.E.
 *
 *      *   The moffs8, moffs16, moffs32 and moffs64 operands specify a simple
 *          offset relative to the segment base, where 8, 16, 32 and 64 refer to
 *          the size of the data.  The address-size attribute of the instruction
 *          determines the size of the offset, either 16, 32 or 64 bits.
 *      **  In 32-bit mode, the assembler may insert the 16-bit operand-size
 *          prefix with this instruction
 *      *** In 64-bit mode, AH, BH, CH, DH cannot be accessed
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(w)    ModRM:reg(r)    NA              NA
 *      RM      ModRM:reg(w)    ModRM:r/m(r)    NA              NA
 *      FD      AL/AX/EAX/RAX   Moffs           NA              NA
 *      TD      Moffs(w)        AL/AX/EAX/RAX   NA              NA
 *      OI      opcode + rd(w)  imm8/16/32/64   NA              NA
 *      MI      ModRM:r/m(w)    imm8/16/32/64   NA              NA
 */
int
_mov(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _is_rm8_r8(val1, val2) ) {
            /* MR Op/En */
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x88;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x89;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x89;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x89;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r8_rm8(val1, val2) ) {
            /* RM Op/En */
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x8a;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r16_rm16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x8b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r32_rm32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x8b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r64_rm64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x8b;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r8_imm8(val1, val2) ) {
            /* OI Op/En */
            ret = _encode_oi(val1, val2, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xb0 + enop.opreg;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r16_imm16(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE16, &enop);
            opsize = SIZE16;
            addrsize = 0;
            opcode1 = 0xb8 + enop.opreg;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r32_imm32(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE32, &enop);
            opsize = SIZE32;
            addrsize = 0;
            opcode1 = 0xb8 + enop.opreg;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_r64_imm64(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE64, &enop);
            opsize = SIZE64;
            addrsize = 0;
            opcode1 = 0xb8 + enop.opreg;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm8_imm8(val1, val2) ) {
            /* MI Op/En */
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xc6;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm16_imm16(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE16, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xc7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm32_imm32(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE32, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xc7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_rm64_imm64(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE64, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0xc7;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * OUT (Vol. 2B 4-131)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      E6 ib           OUT imm8,AL             I       Valid   Valid
 *      E7 ib           OUT imm8,AX             I       Valid   Valid
 *      E7 ib           OUT imm8,EAX            I       Valid   Valid
 *      EE              OUT DX,AL               NP      Valid   Valid
 *      EF              OUT DX,AX               NP      Valid   Valid
 *      EF              OUT DX,EAX              NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      I       imm8            NA              NA              NA
 *      NP      NA              NA              NA              NA
 */
static int
_out(x86_64_target_t target, const operand_vector_t *operands,
    x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _eq_reg(val2, REG_AL) && _is_imm8(val1) ) {
            ret = _encode_i(val1, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe6;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val2, REG_AX) && _is_imm8(val1) ) {
            ret = _encode_i(val1, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val2, REG_EAX) && _is_imm8(val1) ) {
            ret = _encode_i(val1, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xe7;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val2, REG_AL) && _eq_reg(val1, REG_DX) ) {
            ret = 0;
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0xee;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val2, REG_AX) && _eq_reg(val1, REG_DX) ) {
            ret = 0;
            opsize = SIZE16;
            addrsize = 0;
            opcode1 = 0xef;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val2, REG_EAX) && _eq_reg(val1, REG_DX) ) {
            ret = 0;
            opsize = SIZE32;
            addrsize = 0;
            opcode1 = 0xef;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * POPCNT (Vol. 2B 4-270)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F3 0F B8 /r     POPCNT r16,r/m16        RM      Valid   Valid
 *      F3 0F B8 /r     POPCNT r32,r/m32        RM      Valid   Valid
 *      F3 REX.W 0F B8 /r
 *                      POPCNT r64,r/m64        RM      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(w)    ModRM:r/m(r)
 */
int
_popcnt(x86_64_target_t target, const operand_vector_t *operands,
        x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _is_r16_rm16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0xf3;
            opcode2 = 0x0f;
            opcode3 = 0xb8;
        } else if ( _is_r32_rm32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0xf3;
            opcode2 = 0x0f;
            opcode3 = 0xb8;
        } else if ( _is_r64_rm64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0xf3;
            opcode2 = 0x0f;
            opcode3 = 0xb8;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else {
        return -EOPERAND;
    }
}

/*
 * XOR (Vol. 2B 4-531)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      34 ib           XOR AL,imm8             I       Valid   Valid
 *      35 iw           XOR AX,imm16            I       Valid   Valid
 *      35 id           XOR EAX,imm32           I       Valid   Valid
 *      REX.W + 35 id   XOR RAX,imm32           I       Valid   N.E.
 *      80 /6 ib        XOR r/m8,imm8           MI      Valid   Valid
 *      REX + 80 /6 ib  XOR r/m8*,imm8          MI      Valid   N.E.
 *      81 /6 iw        XOR r/m16,imm16         MI      Valid   Valid
 *      81 /6 id        XOR r/m32,imm32         MI      Valid   Valid
 *      REX.W + 81 /6 id
 *                      XOR r/m64,imm32         MI      Valid   N.E.
 *      83 /6 ib        XOR r/m16,imm8          MI      Valid   Valid
 *      83 /6 ib        XOR r/m32,imm8          MI      Valid   Valid
 *      REX.W + 83 /6 ib
 *                      XOR r/m64,imm8          MI      Valid   N.E.
 *      30 /r           XOR r/m8,r8             MR      Valid   Valid
 *      REX + 30 /r     XOR r/m8*,r8*           MR      Valid   N.E.
 *      31 /r           XOR r/m16,r16           MR      Valid   Valid
 *      31 /r           XOR r/m32,r32           MR      Valid   Valid
 *      REX.W + 31 /r   XOR r/m64,r64           MR      Valid   N.E.
 *      32 /r           XOR r8,r/m8             RM      Valid   Valid
 *      REX + 32 /r     XOR r8*,r/m8*           RM      Valid   N.E.
 *      33 /r           XOR r16,r/m16           RM      Valid   Valid
 *      33 /r           XOR r32,r/m32           RM      Valid   Valid
 *      REX.W + 33 /r   XOR r64,r/m64           RM      Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 *      MI      ModRM:r/m(w)    imm8/16/32      NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 */
int
_xor(x86_64_target_t target, const operand_vector_t *operands,
     x86_64_instr_t *instr)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;
    size_t opsize;
    size_t addrsize;
    int opcode1;
    int opcode2;
    int opcode3;

    if ( 2 == mvector_size(operands) ) {
        op1 = mvector_at(operands, 0);
        op2 = mvector_at(operands, 1);

        val1 = x86_64_eval_operand(op1);
        if ( NULL == val1 ) {
            /* Error */
            return -1;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return -1;
        }

        if ( _eq_reg(val1, REG_AL) && _is_imm8(val2) ) {
            ret = _encode_i(val2, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = 0;
            opcode1 = 0x34;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_AX) && _is_imm16(val2) ) {
            ret = _encode_i(val2, SIZE16, &enop);
            opsize = SIZE16;
            addrsize = 0;
            opcode1 = 0x35;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_EAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            opsize = SIZE32;
            addrsize = 0;
            opcode1 = 0x35;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _eq_reg(val1, REG_RAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            opsize = SIZE64;
            addrsize = 0;
            opcode1 = 0x35;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr8(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x80;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x83;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val1) && _is_imm16(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE16, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x81;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x83;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE32, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x81;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x83;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE32, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x81;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr8(val1) && _is_reg8(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x30;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr16(val1) && _is_reg16(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x31;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr32(val1) && _is_reg32(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x31;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg_addr64(val1) && _is_reg64(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val1);
            opcode1 = 0x31;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg8(val1) && _is_reg_addr8(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE8;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x32;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg16(val1) && _is_reg_addr16(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE16;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x33;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg32(val1) && _is_reg_addr32(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE32;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x33;
            opcode2 = -1;
            opcode3 = -1;
        } else if ( _is_reg64(val1) && _is_reg_addr64(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            opsize = SIZE64;
            addrsize = _resolve_address_size1(val2);
            opcode1 = 0x33;
            opcode2 = -1;
            opcode3 = -1;
        } else {
            ret = -1;
        }

        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        ret = _build_instruction(target, &enop, opsize, addrsize, instr);
        if ( ret < 0 ) {
            free(val1);
            free(val2);
            return -EOPERAND;
        }
        instr->opcode1 = opcode1;
        instr->opcode2 = opcode2;
        instr->opcode3 = opcode3;

        free(val1);
        free(val2);

        return 0;
    } else {
        return -EOPERAND;
    }
}


/*
 * Assemble x86-64 code
 */
int
arch_assemble_x86_64(stmt_vector_t *vec)
{
    size_t i;
    stmt_t *stmt;
    int ret;
    x86_64_target_t target;
    x86_64_instr_t instr;

    /* 64 bit option */
    target = X86_64_O64;

    for ( i = 0; i < mvector_size(vec); i++ ){
        stmt = mvector_at(vec, i);
        if ( STMT_INSTR == stmt->type ) {
            if ( 0 == strcasecmp("add", stmt->u.instr->opcode) ) {
                /* ADD */
                ret = _add(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("and", stmt->u.instr->opcode) ) {
                /* AND */
                ret = _and(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("bsf", stmt->u.instr->opcode) ) {
                /* BSF */
                ret = _bsf(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("bsr", stmt->u.instr->opcode) ) {
                /* BSR */
                ret = _bsr(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("bswap", stmt->u.instr->opcode) ) {
                /* BSWAP */
                ret = _bswap(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("bt", stmt->u.instr->opcode) ) {
                /* BT */
                ret = _bt(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("btc", stmt->u.instr->opcode) ) {
                /* BTC */
                ret = _btc(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("btr", stmt->u.instr->opcode) ) {
                /* BTR */
                ret = _btr(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("bts", stmt->u.instr->opcode) ) {
                /* BTS */
                ret = _bts(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cbw", stmt->u.instr->opcode) ) {
                /* CBW */
                ret = _cbw(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cwde", stmt->u.instr->opcode) ) {
                /* CWDE */
                ret = _cwde(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cdqe", stmt->u.instr->opcode) ) {
                /* CDQE */
                ret = _cdqe(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("clc", stmt->u.instr->opcode) ) {
                /* CLC */
                ret = _clc(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cld", stmt->u.instr->opcode) ) {
                /* CLD */
                ret = _cld(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("clflush", stmt->u.instr->opcode) ) {
                /* CLFLUSH */
                ret = _clflush(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cli", stmt->u.instr->opcode) ) {
                /* CLI */
                ret = _cli(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cmp", stmt->u.instr->opcode) ) {
                /* CMP */
                ret = _cmp(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cmpxchg", stmt->u.instr->opcode) ) {
                /* CMPXCHG */
                ret = _cmpxchg(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cpuid", stmt->u.instr->opcode) ) {
                /* CPUID */
                ret = _cpuid(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("crc32", stmt->u.instr->opcode) ) {
                /* CRC32 */
                ret = _crc32(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cwd", stmt->u.instr->opcode) ) {
                /* CWD */
                ret = _cwd(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cdq", stmt->u.instr->opcode) ) {
                /* CDQ */
                ret = _cdq(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("cqo", stmt->u.instr->opcode) ) {
                /* CQO */
                ret = _cqo(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("daa", stmt->u.instr->opcode) ) {
                /* DAA */
                ret = _daa(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("das", stmt->u.instr->opcode) ) {
                /* DAS */
                ret = _das(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("dec", stmt->u.instr->opcode) ) {
                /* DEC */
                ret = _dec(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("div", stmt->u.instr->opcode) ) {
                /* DIV */
                ret = _div(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("hlt", stmt->u.instr->opcode) ) {
                /* HLT */
                ret = _hlt(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("idiv", stmt->u.instr->opcode) ) {
                /* IDIV */
                ret = _idiv(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("imul", stmt->u.instr->opcode) ) {
                /* IMUL */
                ret = _imul(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("in", stmt->u.instr->opcode) ) {
                /* IN */
                ret = _in(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("inc", stmt->u.instr->opcode) ) {
                /* INC */
                ret = _inc(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("jmp", stmt->u.instr->opcode) ) {
                /* JMP */
                ret = _jmp(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("monitor", stmt->u.instr->opcode) ) {
                /* MONITOR */
                ret = _monitor(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("mov", stmt->u.instr->opcode) ) {
                /* MOV */
                ret = _mov(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("out", stmt->u.instr->opcode) ) {
                /* OUT */
                ret = _out(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("popcnt", stmt->u.instr->opcode) ) {
                /* POPCNT */
                ret = _popcnt(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("xor", stmt->u.instr->opcode) ) {
                /* XOR */
                ret = _xor(target, stmt->u.instr->operands, &instr);
            } else {
                /* Unknown */
                fprintf(stderr, "Unknown instruction: %s\n",
                        stmt->u.instr->opcode);
                ret = -1;
            }
            if ( ret >= 0 ) {
                /*_print_instruction_bin(&instr);*/
                _print_instruction(&instr);
                printf("\n");
            } else {
                /* Error */
                fprintf(stderr, "Error: %s\n", stmt->u.instr->opcode);
            }
        }
    }

    return ret;
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

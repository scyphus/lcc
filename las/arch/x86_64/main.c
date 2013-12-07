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
        if ( SIZE8 == val->opsize ) {
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
        if ( SIZE16 == val->opsize ) {
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
        if ( SIZE32 == val->opsize ) {
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
        if ( SIZE64 == val->opsize ) {
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
        if ( SIZE8 == val->opsize ) {
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
        if ( SIZE16 == val->opsize ) {
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
        if ( SIZE32 == val->opsize ) {
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
        if ( SIZE64 == val->opsize ) {
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
    } else if ( 0 == val->opsize ) {
        if ( val->u.imm >= -128 && val->u.imm <= 127 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE8 == val->opsize ) {
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
    } else if ( 0 == val->opsize ) {
        if ( val->u.imm >= -32768 && val->u.imm <= 32767 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE16 == val->opsize ) {
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
    } else if ( 0 == val->opsize ) {
        if ( val->u.imm >= -2147483648 && val->u.imm <= 2147483647 ) {
            return 1;
        } else {
            return 0;
        }
    } else if ( SIZE32 == val->opsize ) {
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
    } else if ( 0 == val->opsize ) {
        return 1;
    } else if ( SIZE64 == val->opsize ) {
        return 1;
    } else {
        return 0;
    }
}

static __inline__ int
_is_rm8_r8(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE8 == val2->opsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->opsize || SIZE8 == val1->opsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm16_r16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE16 == val2->opsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->opsize || SIZE16 == val1->opsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm32_r32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE32 == val2->opsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->opsize || SIZE32 == val1->opsize) ) {
        return 1;
    }

    return 0;
}
static __inline__ int
_is_rm64_r64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val2->type && SIZE64 == val2->opsize
         && (X86_64_VAL_REG == val1->type || X86_64_VAL_ADDR == val1->type)
         && (0 == val1->opsize || SIZE64 == val1->opsize) ) {
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
_is_r32_rm32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    return _is_rm32_r32(val2, val1);
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
    if ( X86_64_VAL_REG == val1->type && SIZE8 == val1->opsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->opsize ) {
            if ( val2->u.imm >= -128 && val2->u.imm <= 127 ) {
                return 1;
            }
        } else if ( SIZE8 == val2->opsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r16_imm16(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE16 == val1->opsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->opsize ) {
            if ( val2->u.imm >= -32768 && val2->u.imm <= 32767 ) {
                return 1;
            }
        } else if ( SIZE16 == val2->opsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r32_imm32(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE32 == val1->opsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->opsize ) {
            if ( val2->u.imm >= -2147483648 && val2->u.imm <= 2147483647 ) {
                return 1;
            }
        } else if ( SIZE32 == val2->opsize ) {
            return 1;
        }
    }

    return 0;
}
static __inline__ int
_is_r64_imm64(const x86_64_val_t *val1, const x86_64_val_t *val2)
{
    /* Check the first and second operands */
    if ( X86_64_VAL_REG == val1->type && SIZE64 == val1->opsize
         && X86_64_VAL_IMM == val2->type ) {
        if ( 0 == val2->opsize ) {
            return 1;
        } else if ( SIZE64 == val2->opsize ) {
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
        if ( SIZE8 == val1->opsize ) {
            if ( 0 == val2->opsize ) {
                if ( val2->u.imm >= -128 && val2->u.imm <= 127 ) {
                    return 1;
                }
            } else if ( SIZE8 == val2->opsize ) {
                return 1;
            }
        } else if ( 0 == val1->opsize ) {
            if ( SIZE8 == val2->opsize ) {
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
        if ( SIZE16 == val1->opsize ) {
            if ( 0 == val2->opsize ) {
                if ( val2->u.imm >= -32768 && val2->u.imm <= 32767 ) {
                    return 1;
                }
            } else if ( SIZE16 == val2->opsize ) {
                return 1;
            }
        } else if ( 0 == val1->opsize ) {
            if ( SIZE16 == val2->opsize ) {
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
        if ( SIZE32 == val1->opsize ) {
            if ( 0 == val2->opsize ) {
                if ( val2->u.imm >= -2147483648 && val2->u.imm <= 2147483647 ) {
                    return 1;
                }
            } else if ( SIZE32 == val2->opsize ) {
                return 1;
            }
        } else if ( 0 == val1->opsize ) {
            if ( SIZE32 == val2->opsize ) {
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
        if ( SIZE64 == val1->opsize ) {
            if ( 0 == val2->opsize ) {
                return 1;
            } else if ( SIZE64 == val2->opsize ) {
                return 1;
            }
        } else if ( 0 == val1->opsize ) {
            if ( SIZE64 == val2->opsize ) {
                return 1;
            }
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
        /* REX is not encodable but has REX */
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
        enop->rex.w = REX_NONE;
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
        enop->rex.w = REX_NONE;
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
    enop->rex.w = REX_NONE;
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
    enop->rex.w = REX_NONE;
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
    //size_t opsz;
    //size_t addrsz;
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
    return _encode_rm(val1, val2, enop);
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
    enop->rex.w = REX_NONE;
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
    enop->rex.w = REX_NONE;
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
    enop->rex.w = REX_NONE;
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
_add(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 04 %.2X\n", (int8_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_AX) && _is_imm16(val2) ) {
            ret = _encode_i(val2, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 66 05 %.4X\n", (int16_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_EAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 05 %.8X\n", (int32_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_RAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD REX.W + 05 %.8X\n", (int32_t)enop.imm.val);
            }
        } else if ( _is_reg_addr8(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 80 %.2X %.2X\n", enop.modrm, (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 66 83 %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm16(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 66 81 %.2X %.4X\n", enop.modrm,
                       (int16_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 83 %.2X %.2X\n", enop.modrm, (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 81 %.2X %.8X\n", enop.modrm, (int32_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD REX.W + 83 %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD REX.W + 81 %.2X %.16llX\n", enop.modrm,
                       enop.imm.val);
            }
        } else if ( _is_reg_addr8(val1) && _is_reg8(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 00 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_reg16(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 66 01 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr32(val1) && _is_reg32(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 01 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr64(val1) && _is_reg64(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD REX.W + 01 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg8(val1) && _is_reg_addr8(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 02 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg16(val1) && _is_reg_addr16(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 66 03 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg32(val1) && _is_reg_addr32(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD 03 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg64(val1) && _is_reg_addr64(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("ADD REX.W + 03 %.2X\n", enop.modrm);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      MI      ModRM:r/m(r,w)  imm8/16/32      NA              NA
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 */
int
_and(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 24 %.2X\n", (int8_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_AX) && _is_imm16(val2) ) {
            ret = _encode_i(val2, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 66 25 %.4X\n", (int16_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_EAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 25 %.8X\n", (int32_t)enop.imm.val);
            }
        } else if ( _eq_reg(val1, REG_RAX) && _is_imm32(val2) ) {
            ret = _encode_i(val2, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND REX.W + 25 %.8X\n", (int32_t)enop.imm.val);
            }
        } else if ( _is_reg_addr8(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 80 %.2X %.2X\n", enop.modrm, (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 66 83 %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm16(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 66 81 %.2X %.4X\n", enop.modrm,
                       (int16_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 83 %.2X %.2X\n", enop.modrm, (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 81 %.2X %.8X\n", enop.modrm, (int32_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND REX.W + 83 %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm32(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND REX.W + 81 %.2X %.16llX\n", enop.modrm,
                       enop.imm.val);
            }
        } else if ( _is_reg_addr8(val1) && _is_reg8(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 20 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_reg16(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 66 21 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr32(val1) && _is_reg32(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 21 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr64(val1) && _is_reg64(val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND REX.W + 21 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg8(val1) && _is_reg_addr8(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 22 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg16(val1) && _is_reg_addr16(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 66 23 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg32(val1) && _is_reg_addr32(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND 23 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg64(val1) && _is_reg_addr64(val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("AND REX.W + 23 %.2X\n", enop.modrm);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
int
_bsf(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSF 66 0F BC %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSF 0F BC %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSF REX.W + 0F BC %.2X\n", enop.modrm);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
int
_bsr(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSR 66 0F BD %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSR 0F BD %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSR REX.W + 0F BD %.2X\n", enop.modrm);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
}

/*
 * BSWAP (Vol. 2A 3-78)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      0F C8+rd        BSWAP r32               O       Valid   Valid
 *      REX.W + 0F C8+rd
 *                      BSWAP r64               O      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      O       opcode+rd(r,w)  NA              NA              NA
 */
int
_bswap(operand_vector_t *operands)
{
    operand_t *op;
    x86_64_val_t *val;
    int ret;
    x86_64_enop_t enop;

    if ( 1 == mvector_size(operands) ) {
        op = mvector_at(operands, 0);

        val = x86_64_eval_operand(op);
        if ( NULL == val ) {
            /* Error */
            return -1;
        }

        if ( _is_reg32(val) ) {
            ret = _encode_o(val, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSWAP 0F %.2X\n", 0xc8 + enop.opreg);
            }
        } else if ( _is_reg64(val) ) {
            ret = _encode_o(val, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BSWAP REX.W + 0F %.2X\n", 0xc8 + enop.opreg);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
int
_bt(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT 66 0F A3 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT 0F A3 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT REX.W + 0F A3 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT 66 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 4, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BT REX.W + 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
_btc(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC 66 0F BB %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC 0F BB %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC REX.W + 0F BB %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 7, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC 66 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 7, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 7, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTC REX.W + 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
_btr(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR 66 0F B3 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR 0F B3 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR REX.W + 0F B3 %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR 66 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 6, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTR REX.W + 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
_bts(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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

        if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS 66 0F AB %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS 0F AB %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS REX.W + 0F AB %.2X\n", enop.modrm);
            }
        } else if ( _is_reg_addr16(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 5, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS 66 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr32(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 5, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else if ( _is_reg_addr64(val1) && _is_imm8(val2) ) {
            ret = _encode_mi(val1, val2, 5, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("BTS REX.W + 0F BA %.2X %.2X\n", enop.modrm,
                       (int8_t)enop.imm.val);
            }
        } else {
            printf("Invalid operands\n");
        }
        free(val1);
        free(val2);
    } else {
        printf("Invalid operands\n");
        return -1;
    }

    return 0;
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
_cbw(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("CBW 66 98\n");

    return 0;
}
int
_cwde(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("CWDE 98\n");

    return 0;
}
int
_cdqe(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("CDQE REX.W + 98\n");

    return 0;
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
_clc(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("CLC F8\n");

    return 0;
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
_cld(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("CLD FC\n");

    return 0;
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
_jmp(operand_vector_t *operands)
{
    if ( 1 != mvector_size(operands) ) {
        return -1;
    }
    printf("JMP\n");

    return 0;
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
_monitor(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return -1;
    }
    printf("MONITOR 0F 01 C8\n");

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
_mov(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int ret;
    x86_64_enop_t enop;

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
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 88 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm16_r16(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 66 89 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm32_r32(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 89 %.2X\n", enop.modrm);
            }
        } else if ( _is_rm64_r64(val1, val2) ) {
            ret = _encode_mr(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                /* REX.W = (1<<3) */
                printf("MOV REX.W 89 %.2X\n", enop.modrm);
            }
        } else if ( _is_r8_rm8(val1, val2) ) {
            /* RM Op/En */
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 8A %.2X\n", enop.modrm);
            }
        } else if ( _is_r16_rm16(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 66 8B %.2X\n", enop.modrm);
            }
        } else if ( _is_r32_rm32(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 8B %.2X\n", enop.modrm);
            }
        } else if ( _is_r64_rm64(val1, val2) ) {
            ret = _encode_rm(val1, val2, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV REX.W 8B %.2X\n", enop.modrm);
            }
        } else if ( _is_r8_imm8(val1, val2) ) {
            /* OI Op/En */
            ret = _encode_oi(val1, val2, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV %.2X %.2llX\n", 0xb0 + enop.opreg, enop.imm.val);
            }
        } else if ( _is_r16_imm16(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 66 %.2X %.4llX\n", 0xb8 + enop.opreg, enop.imm.val);
            }
        } else if ( _is_r32_imm32(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV %.2X %.8llX\n", 0xb8 + enop.opreg, enop.imm.val);
            }
        } else if ( _is_r64_imm64(val1, val2) ) {
            ret = _encode_oi(val1, val2, SIZE64, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV REX.W %.2X %.8llX\n", 0xb8 + enop.opreg,
                       enop.imm.val);
            }
        } else if ( _is_rm8_imm8(val1, val2) ) {
            /* MI Op/En */
            ret = _encode_mi(val1, val2, 0, SIZE8, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV C6 %.2llX\n", enop.imm.val);
            }
        } else if ( _is_rm16_imm16(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE16, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV 66 C7 %.4llX\n", enop.imm.val);
            }
        } else if ( _is_rm32_imm32(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE32, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV C7 %.8llX\n", enop.imm.val);
            }
        } else if ( _is_rm64_imm64(val1, val2) ) {
            ret = _encode_mi(val1, val2, 0, SIZE64, &enop);
            if ( ret < 0 ) {
                printf("Invalid operands\n");
            } else {
                printf("MOV REX.W C7 %.8llX\n", enop.imm.val);
            }
        } else {
            printf("Invalid operands\n");
        }

        free(val1);
        free(val2);
    }

    return 0;
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
_popcnt(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int opsize;
    int ret;
    x86_64_enop_t enop;

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

        if ( val1->opsize == val2->opsize ) {
            /* Operand size matches, but may be zero */
            opsize = val1->opsize;
        } else if ( 0 == val1->opsize && 0 != val2->opsize ) {
            /* Operand size is adjusted to that of Operand2 */
            opsize = val2->opsize;
        } else if ( 0 != val1->opsize && 0 == val2->opsize ) {
            /* Operand size is adjusted to that of Operand1 */
            opsize = val1->opsize;
        } else {
            /* Operand size mismatches */
            opsize = -1;
        }

        if ( opsize <= 0 ) {
            /* Invalid operands (opsize: 0 for unknown, -1 for mismatch) */
            printf("Invalid operands\n");
            free(val1);
            free(val2);
            return -1;
        }

        if ( X86_64_VAL_REG == val1->type && X86_64_VAL_REG == val2->type ) {
            if ( SIZE16 == opsize ) {
                ret = _encode_rm(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("POPCNT F3 0F B8 %X\n", enop.modrm);
                }
            } else if ( SIZE32 == opsize ) {
                ret = _encode_rm(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("POPCNT F3 0F B8 %X\n", enop.modrm);
                }
            } else if ( SIZE64 == opsize ) {
                ret = _encode_rm(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("POPCNT F3 REX.W 0F B8 %X\n", enop.modrm);
                }
            } else {
                printf("Unsupported\n");
            }
        }

        free(val1);
        free(val2);
    }

    return 0;
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
_xor(operand_vector_t *operands)
{
    operand_t *op1;
    operand_t *op2;
    x86_64_val_t *val1;
    x86_64_val_t *val2;
    int opsize;
    int ret;
    x86_64_enop_t enop;

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

        if ( val1->opsize == val2->opsize ) {
            /* Operand size matches, but may be zero */
            opsize = val1->opsize;
        } else if ( 0 == val1->opsize && 0 != val2->opsize ) {
            /* Operand size is adjusted to that of Operand2 */
            opsize = val2->opsize;
        } else if ( 0 != val1->opsize && 0 == val2->opsize ) {
            /* Operand size is adjusted to that of Operand1 */
            opsize = val1->opsize;
        } else {
            /* Operand size mismatches */
            opsize = -1;
        }

        if ( opsize <= 0 ) {
            /* Invalid operands (opsize: 0 for unknown, -1 for mismatch) */
            printf("Invalid operands\n");
            free(val1);
            free(val2);
            return -1;
        }

        if ( X86_64_VAL_REG == val1->type && REG_AL == val1->u.reg
             && X86_64_VAL_IMM == val2->type ) {
            printf("XOR 34 %llX\n", val2->u.imm);
        } else if ( X86_64_VAL_REG == val1->type && REG_AX == val1->u.reg
                    && X86_64_VAL_IMM == val2->type ) {
            printf("XOR 66 35 %llX\n", val2->u.imm);
        } else if ( X86_64_VAL_REG == val1->type && REG_EAX == val1->u.reg
                    && X86_64_VAL_IMM == val2->type ) {
            printf("XOR 35 %llX\n", val2->u.imm);
        } else if ( X86_64_VAL_REG == val1->type && REG_RAX == val1->u.reg
                    && X86_64_VAL_IMM == val2->type ) {
            /* REX.W = (1<<3) */
            printf("XOR REX.W 35 %llX\n", val2->u.imm);
        } else if ( X86_64_VAL_REG == val1->type
                    && X86_64_VAL_REG == val2->type ) {
            if ( SIZE16 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("XOR 66 31 %X\n", enop.modrm);
                }
            } else if ( SIZE32 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("XOR 31 %X\n", enop.modrm);
                }
            } else if ( SIZE64 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("XOR REX.W 31 %X\n", enop.modrm);
                }
            } else {
                printf("Unsupported\n");
            }
        }

        free(val1);
        free(val2);
    }

    return 0;
}


/*
 * Assemble x86-64 code
 */
void
arch_assemble_x86_64(stmt_vector_t *vec)
{
    size_t i;
    stmt_t *stmt;
    int ret;

    for ( i = 0; i < mvector_size(vec); i++ ){
        stmt = mvector_at(vec, i);
        if ( STMT_INSTR == stmt->type ) {
            if ( 0 == strcasecmp("add", stmt->u.instr->opcode) ) {
                /* ADD */
                ret = _add(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("and", stmt->u.instr->opcode) ) {
                /* AND */
                ret = _and(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("bsf", stmt->u.instr->opcode) ) {
                /* BSF */
                ret = _bsf(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("bsr", stmt->u.instr->opcode) ) {
                /* BSR */
                ret = _bsr(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("bswap", stmt->u.instr->opcode) ) {
                /* BSWAP */
                ret = _bswap(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("bt", stmt->u.instr->opcode) ) {
                /* BT */
                ret = _bt(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("btc", stmt->u.instr->opcode) ) {
                /* BTC */
                ret = _btc(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("btr", stmt->u.instr->opcode) ) {
                /* BTR */
                ret = _btr(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("bts", stmt->u.instr->opcode) ) {
                /* BTS */
                ret = _bts(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("cbw", stmt->u.instr->opcode) ) {
                /* CBW */
                ret = _cbw(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("cwde", stmt->u.instr->opcode) ) {
                /* CWDE */
                ret = _cwde(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("cdqe", stmt->u.instr->opcode) ) {
                /* CDQE */
                ret = _cdqe(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("clc", stmt->u.instr->opcode) ) {
                /* CLC */
                ret = _clc(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("cld", stmt->u.instr->opcode) ) {
                /* CLD */
                ret = _cld(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("jmp", stmt->u.instr->opcode) ) {
                /* JMP */
                ret = _jmp(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("monitor", stmt->u.instr->opcode) ) {
                /* MONITOR */
                ret = _monitor(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("mov", stmt->u.instr->opcode) ) {
                /* MOV */
                ret = _mov(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("popcnt", stmt->u.instr->opcode) ) {
                /* POPCNT */
                ret = _popcnt(stmt->u.instr->operands);
            } else if ( 0 == strcasecmp("xor", stmt->u.instr->opcode) ) {
                /* XOR */
                ret = _xor(stmt->u.instr->operands);
            } else {
                /* Unknown */
                printf("Unknown instruction: %s\n", stmt->u.instr->opcode);
                ret = 0;
            }
        }
    }

    return;
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

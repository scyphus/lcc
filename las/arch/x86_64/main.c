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

#define EADDR32         32
#define EADDR64         64

#define OPSIZE8         8
#define OPSIZE16        16
#define OPSIZE32        32
#define OPSIZE64        64



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



/*
 * CLC (Vol. 2A 3-101)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F8              CLC                     NP      Valid   Valid
 */
void
_clc(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return;
    }
    printf("CLC F8\n");
}


/*
 * CLD (Vol. 2A 3-102)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FC              CLD                     NP      Valid   Valid
 */
void
_cld(operand_vector_t *operands)
{
    if ( 0 != mvector_size(operands) ) {
        return;
    }
    printf("CLD FC\n");
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
void
_jmp(operand_vector_t *operands)
{
    if ( 1 != mvector_size(operands) ) {
        return;
    }
    printf("JMP\n");
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
void
_mov(operand_vector_t *operands)
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
            return;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return;
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
            return;
        }


        if ( X86_64_VAL_REG == val1->type && X86_64_VAL_REG == val2->type ) {
            if ( 8 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("MOV 88 %X\n", enop.modrm);
                }
            } else if ( 16 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("MOV 66 89 %X\n", enop.modrm);
                }
            } else if ( 32 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("MOV 89 %X\n", enop.modrm);
                }
            } else if ( 64 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    /* REX.W = (1<<3) */
                    printf("MOV REX.W 89 %X\n", enop.modrm);
                }
            } else {
                printf("Unsupported\n");
            }
        } else {
            printf("MOV (#args = %zu)\n", mvector_size(operands));
        }

        free(val1);
        free(val2);
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
void
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
            return;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return;
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
            return;
        }

        if ( X86_64_VAL_REG == val1->type && X86_64_VAL_REG == val2->type ) {
            if ( 16 == opsize ) {
                ret = _encode_rm(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("POPCNT F3 0F B8 %X\n", enop.modrm);
                }
            } else if ( 32 == opsize ) {
                ret = _encode_rm(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("POPCNT F3 0F B8 %X\n", enop.modrm);
                }
            } else if ( 64 == opsize ) {
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
void
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
            return;
        }
        val2 = x86_64_eval_operand(op2);
        if ( NULL == val2 ) {
            /* Error */
            free(val1);
            return;
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
            return;
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
            if ( 16 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("XOR 66 31 %X\n", enop.modrm);
                }
            } else if ( 32 == opsize ) {
                ret = _encode_mr(val1, val2, &enop);
                if ( ret < 0 ) {
                    printf("Invalid operands\n");
                } else {
                    printf("XOR 31 %X\n", enop.modrm);
                }
            } else if ( 64 == opsize ) {
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
}


/*
 * Assemble x86-64 code
 */
void
arch_assemble_x86_64(stmt_vector_t *vec)
{
    size_t i;
    stmt_t *stmt;

    for ( i = 0; i < mvector_size(vec); i++ ){
        stmt = mvector_at(vec, i);
        if ( STMT_INSTR == stmt->type ) {
            if ( 0 == strcmp("clc", stmt->u.instr->opcode) ) {
                /* CLC */
                _clc(stmt->u.instr->operands);
            } else if ( 0 == strcmp("cld", stmt->u.instr->opcode) ) {
                /* CLD */
                _cld(stmt->u.instr->operands);
            } else if ( 0 == strcmp("jmp", stmt->u.instr->opcode) ) {
                /* JMP */
                _jmp(stmt->u.instr->operands);
            } else if ( 0 == strcmp("mov", stmt->u.instr->opcode) ) {
                /* MOV */
                _mov(stmt->u.instr->operands);
            } else if ( 0 == strcmp("popcnt", stmt->u.instr->opcode) ) {
                /* POPCNT */
                _popcnt(stmt->u.instr->operands);
            } else if ( 0 == strcmp("xor", stmt->u.instr->opcode) ) {
                /* XOR */
                _xor(stmt->u.instr->operands);
            } else {
                /* Unknown */
                printf("Unknown instruction: %s\n", stmt->u.instr->opcode);
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

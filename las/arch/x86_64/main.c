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

/*
 * Check the size of the immediate value
 */
static int
_check_disp_size(int64_t i)
{
    if ( i > 127 || i < -128 ) {
        return 32;
    } else {
        return 8;
    }
}


#define REXB_NE         0
#define REXB_NONE       0       /* Not encodable */
#define REXB_YES        1

/*
 * Resolve register code (and REX.B)
 * Vol. 2A 3-2
 */
static int
_reg_code(x86_64_reg_t reg, int *code, int *rexb)
{
    switch ( reg ) {
    case REG_AL:
    case REG_AX:
    case REG_EAX:
    case REG_RAX:
        *code = 0;
        *rexb = REXB_NONE;
        break;
    case REG_CL:
    case REG_CX:
    case REG_ECX:
    case REG_RCX:
        *code = 1;
        *rexb = REXB_NONE;
        break;
    case REG_DL:
    case REG_DX:
    case REG_EDX:
    case REG_RDX:
        *code = 2;
        *rexb = REXB_NONE;
        break;
    case REG_BL:
    case REG_BX:
    case REG_EBX:
    case REG_RBX:
        *code = 3;
        *rexb = REXB_NONE;
        break;
    case REG_AH:
        *code = 4;
        *rexb = REXB_NE;
        break;
    case REG_SP:
    case REG_ESP:
    case REG_RSP:
        *code = 4;
        *rexb = REXB_NONE;
        break;
    case REG_SPL:
        *code = 4;
        *rexb = REXB_YES;
        break;
    case REG_CH:
        *code = 5;
        *rexb = REXB_NE;
        break;
    case REG_BP:
    case REG_EBP:
    case REG_RBP:
        *code = 5;
        *rexb = REXB_NONE;
        break;
    case REG_BPL:
        *code = 5;
        *rexb = REXB_YES;
        break;
    case REG_DH:
        *code = 6;
        *rexb = REXB_NE;
        break;
    case REG_SI:
    case REG_ESI:
    case REG_RSI:
        *code = 6;
        *rexb = REXB_NONE;
        break;
    case REG_SIL:
        *code = 6;
        *rexb = REXB_YES;
        break;
    case REG_BH:
        *code = 7;
        *rexb = REXB_NE;
        break;
    case REG_DI:
    case REG_EDI:
    case REG_RDI:
        *code = 7;
        *rexb = REXB_NONE;
        break;
    case REG_DIL:
        *code = 7;
        *rexb = REXB_YES;
        break;
    case REG_R8L:
    case REG_R8W:
    case REG_R8D:
    case REG_R8:
        *code = 0;
        *rexb = REXB_YES;
        break;
    case REG_R9L:
    case REG_R9W:
    case REG_R9D:
    case REG_R9:
        *code = 1;
        *rexb = REXB_YES;
        break;
    case REG_R10L:
    case REG_R10W:
    case REG_R10D:
    case REG_R10:
        *code = 2;
        *rexb = REXB_YES;
        break;
    case REG_R11L:
    case REG_R11W:
    case REG_R11D:
    case REG_R11:
        *code = 3;
        *rexb = REXB_YES;
        break;
    case REG_R12L:
    case REG_R12W:
    case REG_R12D:
    case REG_R12:
        *code = 4;
        *rexb = REXB_YES;
        break;
    case REG_R13L:
    case REG_R13W:
    case REG_R13D:
    case REG_R13:
        *code = 5;
        *rexb = REXB_YES;
        break;
    case REG_R14L:
    case REG_R14W:
    case REG_R14D:
    case REG_R14:
        *code = 6;
        *rexb = REXB_YES;
        break;
    case REG_R15L:
    case REG_R15W:
    case REG_R15D:
    case REG_R15:
        *code = 7;
        *rexb = REXB_YES;
        break;
    default:
        *code = -1;
        *rexb = REXB_NE;
    }

    return 0;
}

/* REX prefix = [0100WRXB] */
int
_rex(int w, int r, int x, int b)
{
    return (1<<6) | (w<<3) | (r<<2) | (x<<1) | b;
}

/*
 * Vol. 2A 2-5
 * Mod(2) | Reg(3) | R/M(3)
 */
static int
_val_to_modrm(x86_64_val_t *val1, x86_64_val_t *val2)
{
    int mod;
    int reg;
    int rm;
    int sib;

    /* REG */
    if ( X86_64_VAL_REG != val1->type ) {
        return -1;
    }
    switch ( val1->u.reg ) {
    case REG_AL:
    case REG_AX:
    case REG_EAX:
    case REG_MM0:
    case REG_XMM0:
        reg = 0;
        break;
    case REG_CL:
    case REG_CX:
    case REG_ECX:
    case REG_MM1:
    case REG_XMM1:
        reg = 1;
        break;
    case REG_DL:
    case REG_DX:
    case REG_EDX:
    case REG_MM2:
    case REG_XMM2:
        reg = 2;
        break;
    case REG_BL:
    case REG_BX:
    case REG_EBX:
    case REG_MM3:
    case REG_XMM3:
        reg = 3;
        break;
    case REG_AH:
    case REG_SP:
    case REG_ESP:
    case REG_MM4:
    case REG_XMM4:
        reg = 4;
        break;
    case REG_CH:
    case REG_BP:
    case REG_EBP:
    case REG_MM5:
    case REG_XMM5:
        reg = 5;
        break;
    case REG_DH:
    case REG_SI:
    case REG_ESI:
    case REG_MM6:
    case REG_XMM6:
        reg = 6;
        break;
    case REG_BH:
    case REG_DI:
    case REG_EDI:
    case REG_MM7:
    case REG_XMM7:
        reg = 7;
        break;
    default:
        reg = -1;
    }
    /* Check */
    if ( reg < 0 ) {
        return -1;
    }


    /* R/M */
    if ( X86_64_VAL_REG == val2->type ) {
        mod = 3;
        switch ( val2->u.reg ) {
        case REG_EAX:
        case REG_AX:
        case REG_AL:
        case REG_MM0:
        case REG_XMM0:
            rm = 0;
            break;
        case REG_ECX:
        case REG_CX:
        case REG_CL:
        case REG_MM1:
        case REG_XMM1:
            rm = 1;
            break;
        case REG_EDX:
        case REG_DX:
        case REG_DL:
        case REG_MM2:
        case REG_XMM2:
            rm = 2;
            break;
        case REG_EBX:
        case REG_BX:
        case REG_BL:
        case REG_MM3:
        case REG_XMM3:
            rm = 3;
            break;
        case REG_ESP:
        case REG_SP:
        case REG_AH:
        case REG_MM4:
        case REG_XMM4:
            rm = 4;
            break;
        case REG_EBP:
        case REG_BP:
        case REG_CH:
        case REG_MM5:
        case REG_XMM5:
            rm = 5;
            break;
        case REG_ESI:
        case REG_SI:
        case REG_DH:
        case REG_MM6:
        case REG_XMM6:
            rm = 6;
            break;
        case REG_EDI:
        case REG_DI:
        case REG_BH:
        case REG_MM7:
        case REG_XMM7:
            rm = 7;
            break;
        default:
            rm = -1;
        }
        /* Check */
        if ( rm < 0 ) {
            return -1;
        }
    } else if ( X86_64_VAL_ADDR == val2->type ) {
        /* Address */

        /* Displacement? */
        if ( val2->u.addr.flags & X86_64_ADDR_DISP ) {
            /* Has disp */
            if ( 0 == val2->u.addr.disp ) {
                mod = 0;
            } else if ( 8 == _check_disp_size(val2->u.addr.disp) ) {
                mod = 1;
            } else {
                mod = 2;
            }
        } else {
            mod = 0;
        }

        /* SIB? */
        if ( val2->u.addr.flags & X86_64_ADDR_OFFSET ) {
            /* Has offset (SIB) */
        } else {
            /* Base register */
            if ( val2->u.addr.flags & X86_64_ADDR_BASE ) {
                switch ( val2->u.addr.base ) {
                case REG_EAX:
                    /* Add effective address size prefix */
                    rm = 0;
                    break;
                case REG_RAX:
                    rm = 0;
                    break;
                case REG_ECX:
                    /* Add effective address size prefix */
                    rm = 1;
                    break;
                case REG_RCX:
                    rm = 1;
                    break;
                case REG_EDX:
                    /* Add effective address size prefix */
                    rm = 2;
                    break;
                case REG_RDX:
                    rm = 2;
                    break;
                case REG_EBX:
                    /* Add effective address size prefix */
                    rm = 3;
                    break;
                case REG_RBX:
                    rm = 3;
                    break;
                case REG_ESI:
                    /* Add effective address size prefix */
                    rm = 6;
                    break;
                case REG_RSI:
                    rm = 6;
                    break;
                case REG_EDI:
                    /* Add effective address size prefix */
                    rm = 7;
                    break;
                case REG_RDI:
                    rm = 7;
                    break;
                default:
                    rm = -1;
                }
            } else {
                sib = 0x25;
                rm = 4;
            }
        }
    } else {
        /* Immediate */
        return -1;
    }

    return (mod << 6) | (reg << 3) | rm;
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
                printf("MOV 88 %X\n", _val_to_modrm(val1, val2));
            } else if ( 16 == opsize ) {
                printf("MOV 66 89 %X\n", _val_to_modrm(val1, val2));
            } else if ( 32 == opsize ) {
                printf("MOV 89 %X\n", _val_to_modrm(val1, val2));
            } else if ( 64 == opsize ) {
                /* REX.W = (1<<3) */
                printf("MOV REX.W 89 %X\n", _val_to_modrm(val1, val2));
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
                printf("POPCNT F3 0F B8 %X\n", _val_to_modrm(val1, val2));
            } else if ( 32 == opsize ) {
                printf("POPCNT F3 0F B8 %X\n", _val_to_modrm(val1, val2));
            } else if ( 64 == opsize ) {
                printf("POPCNT F3 REX.W 0F B8 %X\n", _val_to_modrm(val1, val2));
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
                printf("XOR 66 31 %X\n", _val_to_modrm(val1, val2));
            } else if ( 32 == opsize ) {
                printf("XOR 31 %X\n", _val_to_modrm(val1, val2));
            } else if ( 64 == opsize ) {
                printf("XOR REX.W 31 %X\n", _val_to_modrm(val1, val2));
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

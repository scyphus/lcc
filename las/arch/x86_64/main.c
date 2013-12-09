/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "../../las.h"
#include "binstr.h"
#include "reg.h"
#include "eval.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>



#define PASS0(ret) do {                         \
    if ( 0 < ret ) { return ret; }              \
    else if ( 0 > ret ) { return 0; }           \
    } while ( 0 )

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
    PASS0(binstr(instr, tgt, SIZE32, 0x0f, 0xc8, -1, -1, ops, ENC_O_R32));
    PASS0(binstr(instr, tgt, SIZE64, 0x0f, 0xc8, -1, -1, ops, ENC_O_R64));

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
    PASS0(binstr(instr, tgt, 0, 0xf8, -1, -1, -1, ops, ENC_NP));

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
    PASS0(binstr(instr, tgt, 0, 0xfc, -1, -1, -1, ops, ENC_NP));
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
    PASS0(binstr(instr, tgt, SIZE8, 0x0f, 0xae, -1, 7, ops, ENC_M_M8));

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
    PASS0(binstr(instr, tgt, 0, 0xfa, -1, -1, -1, ops, ENC_NP));

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
    PASS0(binstr(instr, tgt, 0, 0x0f, 0xa2, -1, -1, ops, ENC_NP));

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
_cwd(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE16, 0x99, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
int
_cdq(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE32, 0x99, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
int
_cqo(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE64, 0x99, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
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
_daa(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, 0, 0x27, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
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
_das(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, 0, 0x2f, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
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
_dec(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xfe, -1, -1, 1, ops, ENC_M_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xff, -1, -1, 1, ops, ENC_M_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xff, -1, -1, 1, ops, ENC_M_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xff, -1, -1, 1, ops, ENC_M_RM64));

    return -EOPERAND;
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
_div(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xf6, -1, -1, 6, ops, ENC_M_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xf7, -1, -1, 6, ops, ENC_M_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xf7, -1, -1, 6, ops, ENC_M_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xf7, -1, -1, 6, ops, ENC_M_RM64));

    return -EOPERAND;
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
_hlt(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, 0, 0xf4, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
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
_idiv(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xf6, -1, -1, 7, ops, ENC_M_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xf7, -1, -1, 7, ops, ENC_M_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xf7, -1, -1, 7, ops, ENC_M_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xf7, -1, -1, 7, ops, ENC_M_RM64));

    return -EOPERAND;
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
_imul(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xf6, -1, -1, 5, ops, ENC_M_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xf7, -1, -1, 5, ops, ENC_M_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xf7, -1, -1, 5, ops, ENC_M_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xf7, -1, -1, 5, ops, ENC_M_RM64));
    PASS0(binstr(instr, tgt, SIZE16, 0x0f, 0xaf, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x0f, 0xaf, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x0f, 0xaf, -1, -1, ops, ENC_RM_R64_RM64));
    PASS0(binstr(instr, tgt, SIZE16, 0x6b, -1, -1, -1, ops,
                 ENC_RMI_R16_RM16_IMM8));

    return -EOPERAND;
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
_in(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xe4, -1, -1, -1, ops, ENC_I_AL_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xe5, -1, -1, -1, ops, ENC_I_AX_IMM8));
    PASS0(binstr(instr, tgt, SIZE32, 0xe5, -1, -1, -1, ops, ENC_I_EAX_IMM8));
    PASS0(binstr(instr, tgt, SIZE8, 0xec, -1, -1, -1, ops, ENC_NP_AL_DX));
    PASS0(binstr(instr, tgt, SIZE16, 0xed, -1, -1, -1, ops, ENC_NP_AX_DX));
    PASS0(binstr(instr, tgt, SIZE32, 0xed, -1, -1, -1, ops, ENC_NP_EAX_DX));

    return -EOPERAND;
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
_inc(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xfe, -1, -1, 0, ops, ENC_M_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xff, -1, -1, 0, ops, ENC_M_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xff, -1, -1, 0, ops, ENC_M_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xff, -1, -1, 0, ops, ENC_M_RM64));

    return -EOPERAND;
}

/*
 * IRET (Vol. 2A 3-411)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      CF              IRET                    NP      Valid   Valid
 *      CF              IRETD                   NP      Valid   Valid
 *      REX.W + CF      IRETQ                   NP      Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
static int
_iret(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE16, 0xcf, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
static int
_iretd(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE32, 0xcf, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
}
static int
_iretq(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE64, 0xcf, -1, -1, -1, ops, ENC_NP));

    return -EOPERAND;
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
_jmp(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
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
_monitor(x86_64_target_t tgt, const operand_vector_t *ops,
         x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, 0, 0x0f, 0x01, 0xc8, -1, ops, ENC_NP));

    return -EOPERAND;
}


/*
 * MOV (Vol. 2B 4-29)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      88 /r           MOV r/m8,r8             MR      Valid   Valid
 *      REX + 88 /r     MOV r/m8***,r8***       MR      Valid   N.E.
 *      89 /r           MOV r/m16,r16           MR      Valid   Valid
 *      89 /r           MOV r/m32,r32           MR      Valid   Valid
 *      REX.W + 89 /r   MOV r/m64,r64           MR      Valid   N.E.
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
_mov(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0x88, -1, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, SIZE16, 0x89, -1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, SIZE32, 0x89, -1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, SIZE64, 0x89, -1, -1, -1, ops, ENC_MR_RM64_R64));

    PASS0(binstr(instr, tgt, SIZE8, 0x8a, -1, -1, -1, ops, ENC_RM_R8_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x8b, -1, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x8b, -1, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x8b, -1, -1, -1, ops, ENC_RM_R64_RM64));

    PASS0(binstr(instr, tgt, SIZE8, 0xb0, -1, -1, -1, ops, ENC_OI_R8_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xb8, -1, -1, -1, ops, ENC_OI_R16_IMM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xb8, -1, -1, -1, ops, ENC_OI_R32_IMM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xb8, -1, -1, -1, ops, ENC_OI_R64_IMM64));

    PASS0(binstr(instr, tgt, SIZE8, 0xc6, -1, -1, 0, ops, ENC_MI_RM8_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0xc7, -1, -1, 0, ops, ENC_MI_RM16_IMM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xc7, -1, -1, 0, ops, ENC_MI_RM32_IMM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xc7, -1, -1, 0, ops, ENC_MI_RM64_IMM64));

    return -EOPERAND;
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
_out(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0xe6, -1, -1, -1, ops, ENC_I_IMM8_AL));
    PASS0(binstr(instr, tgt, SIZE16, 0xe7, -1, -1, -1, ops, ENC_I_IMM8_AX));
    PASS0(binstr(instr, tgt, SIZE32, 0xe7, -1, -1, -1, ops, ENC_I_IMM8_EAX));
    PASS0(binstr(instr, tgt, SIZE8, 0xee, -1, -1, -1, ops, ENC_NP_DX_AL));
    PASS0(binstr(instr, tgt, SIZE16, 0xef, -1, -1, -1, ops, ENC_NP_DX_AL));
    PASS0(binstr(instr, tgt, SIZE32, 0xef, -1, -1, -1, ops, ENC_NP_DX_EAX));

    return -EOPERAND;
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
_popcnt(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE16, 0xf3, 0x0f, 0xb8, -1, ops,
                 ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0xf3, 0x0f, 0xb8, -1, ops,
                 ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0xf3, 0x0f, 0xb8, -1, ops,
                 ENC_RM_R64_RM64));

    return -EOPERAND;
}

/*
 * RET (Vol. 2B 4-369)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      C3              RET                     NP      Valid   Valid
 *      CB              RET                     NP      Valid   Valid
 *      C2 iw           RET imm16               I       Valid   Valid
 *      CA iw           RET imm16               I       Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 *      I       imm16           NA              NA              NA
 */
static int
_ret(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, 0, 0xc3, -1, -1, -1, ops, ENC_NP));
    PASS0(binstr(instr, tgt, 0, 0xc2, -1, -1, -1, ops, ENC_I_IMM16));

#if 0
    /* FIXME: Support "far return" */
    PASS0(binstr(instr, tgt, 0, 0xcb, -1, -1, -1, ops, ENC_NP));
    PASS0(binstr(instr, tgt, 0, 0xca, -1, -1, -1, ops, ENC_I_IMM16));
#endif

    return -EOPERAND;
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
_xor(x86_64_target_t tgt, const operand_vector_t *ops, x86_64_instr_t *instr)
{
    PASS0(binstr(instr, tgt, SIZE8, 0x34, -1, -1, -1, ops, ENC_I_AL_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x35, -1, -1, -1, ops, ENC_I_AX_IMM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x35, -1, -1, -1, ops, ENC_I_EAX_IMM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x35, -1, -1, -1, ops, ENC_I_RAX_IMM32));

    PASS0(binstr(instr, tgt, SIZE8, 0x80, -1, -1, 6, ops, ENC_MI_RM8_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x83, -1, -1, 6, ops, ENC_MI_RM16_IMM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x81, -1, -1, 6, ops, ENC_MI_RM16_IMM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x83, -1, -1, 6, ops, ENC_MI_RM32_IMM8));
    PASS0(binstr(instr, tgt, SIZE32, 0x81, -1, -1, 6, ops, ENC_MI_RM32_IMM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x83, -1, -1, 6, ops, ENC_MI_RM64_IMM8));
    PASS0(binstr(instr, tgt, SIZE64, 0x81, -1, -1, 6, ops, ENC_MI_RM64_IMM32));

    PASS0(binstr(instr, tgt, SIZE8, 0x30, -1, -1, -1, ops, ENC_MR_RM8_R8));
    PASS0(binstr(instr, tgt, SIZE16, 0x31, -1, -1, -1, ops, ENC_MR_RM16_R16));
    PASS0(binstr(instr, tgt, SIZE32, 0x31, -1, -1, -1, ops, ENC_MR_RM32_R32));
    PASS0(binstr(instr, tgt, SIZE64, 0x31, -1, -1, -1, ops, ENC_MR_RM64_R64));

    PASS0(binstr(instr, tgt, SIZE8, 0x32, -1, -1, -1, ops, ENC_RM_R8_RM8));
    PASS0(binstr(instr, tgt, SIZE16, 0x33, -1, -1, -1, ops, ENC_RM_R16_RM16));
    PASS0(binstr(instr, tgt, SIZE32, 0x33, -1, -1, -1, ops, ENC_RM_R32_RM32));
    PASS0(binstr(instr, tgt, SIZE64, 0x33, -1, -1, -1, ops, ENC_RM_R64_RM64));

    return -EOPERAND;
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
            } else if ( 0 == strcasecmp("iret", stmt->u.instr->opcode) ) {
                /* IRET */
                ret = _iret(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("iretd", stmt->u.instr->opcode) ) {
                /* IRETD */
                ret = _iretd(target, stmt->u.instr->operands, &instr);
            } else if ( 0 == strcasecmp("iretq", stmt->u.instr->opcode) ) {
                /* IRETQ */
                ret = _iretq(target, stmt->u.instr->operands, &instr);
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
            } else if ( 0 == strcasecmp("ret", stmt->u.instr->opcode) ) {
                /* RET */
                ret = _ret(target, stmt->u.instr->operands, &instr);
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
                _print_instruction_bin(&instr);
#if 0
                _print_instruction(&instr);
#endif
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

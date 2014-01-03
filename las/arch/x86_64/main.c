/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "../../las.h"
#include "../../token.h"
#include "../../parser.h"
#include "binstr.h"
#include "reg.h"
#include "eval.h"
#include "main.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/* FIXME: Fix the error handling */
#define EC(f) do {                                                      \
        int ret = (f);                                                  \
        if ( ret < 0 && ret != -ESIZE ) { return ret; }                 \
    } while ( 0 )


/*
 * Obtain the size of the instruction
 */
static size_t
_instr_size(const x86_64_instr_t *instr)
{
    size_t sz;

    sz = 0;
    if ( instr->prefix1 >= 0 ) {
        sz++;
    }
    if ( instr->prefix2 >= 0 ) {
        sz++;
    }
    if ( instr->prefix3 >= 0 ) {
        sz++;
    }
    if ( instr->prefix4 >= 0 ) {
        sz++;
    }
    if ( instr->rex >= 0 ) {
        sz++;
    }
    if ( instr->opcode1 >= 0 ) {
        sz++;
    }
    if ( instr->opcode2 >= 0 ) {
        sz++;
    }
    if ( instr->opcode3 >= 0 ) {
        sz++;
    }
    if ( instr->modrm >= 0 ) {
        sz++;
    }
    if ( instr->sib >= 0 ) {
        sz++;
    }
    sz += instr->disp.sz;
    sz += instr->imm.sz;
    sz += instr->rel.sz;

    return sz;
}


/*
 * Print assembled code
 */
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
    val = instr->rel.val;
    for ( i = 0; i < instr->rel.sz; i++ ) {
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
    val = instr->rel.val;
    for ( i = 0; i < instr->rel.sz; i++ ) {
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
_add(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x04, -1, -1, ENC_I_AL_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x05, -1, -1, ENC_I_AX_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x05, -1, -1, ENC_I_EAX_IMM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x05, -1, -1, ENC_I_RAX_IMM32, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x80, -1, -1, ENC_MI_RM8_IMM8, 0));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x81, -1, -1, ENC_MI_RM16_IMM16, 0));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x81, -1, -1, ENC_MI_RM32_IMM32, 0));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x81, -1, -1, ENC_MI_RM64_IMM32, 0));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x83, -1, -1, ENC_MI_RM16_IMM8, 0));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x83, -1, -1, ENC_MI_RM32_IMM8, 0));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x83, -1, -1, ENC_MI_RM64_IMM8, 0));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x00, -1, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x01, -1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x01, -1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x01, -1, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x02, -1, -1, ENC_RM_R8_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x03, -1, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x03, -1, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x03, -1, -1, ENC_RM_R64_RM64, -1));

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
_and(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x24, -1, -1, ENC_I_AL_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x25, -1, -1, ENC_I_AX_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x25, -1, -1, ENC_I_EAX_IMM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x25, -1, -1, ENC_I_RAX_IMM32, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x80, -1, -1, ENC_MI_RM8_IMM8, 4));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x81, -1, -1, ENC_MI_RM16_IMM16, 4));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x81, -1, -1, ENC_MI_RM32_IMM32, 4));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x81, -1, -1, ENC_MI_RM64_IMM32, 4));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x83, -1, -1, ENC_MI_RM16_IMM8, 4));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x83, -1, -1, ENC_MI_RM32_IMM8, 4));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x83, -1, -1, ENC_MI_RM64_IMM8, 4));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x20, -1, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x21, -1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x21, -1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x21, -1, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x22, -1, -1, ENC_RM_R8_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x23, -1, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x23, -1, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x23, -1, -1, ENC_RM_R64_RM64, -1));

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
static int
_bsf(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xbc, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xbc, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xbc, -1, ENC_RM_R64_RM64, -1));

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
static int
_bsr(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xbd, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xbd, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xbd, -1, ENC_RM_R64_RM64, -1));

    return 0;
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
_bswap(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xc8, -1, ENC_O_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xc8, -1, ENC_O_R64, -1));

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
static int
_bt(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xa3, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xa3, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xa3, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xba, -1, ENC_MI_RM16_IMM8, 4));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xba, -1, ENC_MI_RM32_IMM8, 4));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xba, -1, ENC_MI_RM64_IMM8, 4));

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
static int
_btc(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xbb, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xbb, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xbb, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xba, -1, ENC_MI_RM16_IMM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xba, -1, ENC_MI_RM32_IMM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xba, -1, ENC_MI_RM64_IMM8, 7));

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
static int
_btr(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xb3, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xb3, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xb3, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xba, -1, ENC_MI_RM16_IMM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xba, -1, ENC_MI_RM32_IMM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xba, -1, ENC_MI_RM64_IMM8, 6));

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
static int
_bts(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xab, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xab, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xab, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xba, -1, ENC_MI_RM16_IMM8, 5));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xba, -1, ENC_MI_RM32_IMM8, 5));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xba, -1, ENC_MI_RM64_IMM8, 5));

    return 0;
}

/*
 * CALL (Vol. 2A 3-87)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      E8 cw           CALL rel16              D       N.S.    Valid
 *      E8 cd           CALL rel32              D       Valid   Valid
 *      FF /2           CALL r/m16              M       N.S.    Valid
 *      FF /2           CALL r/m32              M       N.S.    Valid
 *      FF /2           CALL r/m64              M       Valid   N.E.
 *      9A cd           CALL ptr16:16           D       Inv.    Valid
 *      9A cp           CALL ptr16:32           D       Inv.    Valid
 *      FF /3           CALL m16:16             D       Valid   Valid
 *      FF /3           CALL m16:32             D       Valid   Valid
 *      REX.W + FF /3   CALL m16:64             D       Valid   N.E.
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      D       Offset          NA              NA              NA
 *      M       ModR/M(r)       NA              NA              NA
 */
static int
_call(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    if ( OPCODE_SUFFIX_FAR & xstmt->suffix ) {
        /* w/ far */

        /* To be implemented */
        return -EUNKNOWN;
    } else {
        /* w/o far */
        EC(binstr2(asmblr, xstmt, 0, 0xe8, -1, -1, ENC_D_REL32, -1));
        EC(binstr2(asmblr, xstmt, 0, 0xff, -1, -1, ENC_M_RM64, 2));
        /* Invalid for 64-bit mode */
        /*EC(binstr2(asmblr, xstmt, SIZE16, 0xe8, -1, -1, ENC_D_REL16, -1));*/
        /*EC(binstr2(asmblr, xstmt, SIZE16, 0xff, -1, -1, ENC_M_RM16, 2));*/
        /*EC(binstr2(asmblr, xstmt, SIZE32, 0xff, -1, -1, ENC_M_RM32, 2));*/

        return 0;
    }
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
static int
_cbw(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x98, -1, -1, ENC_NP, -1));

    return 0;
}
static int
_cwde(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE32, 0x98, -1, -1, ENC_NP, -1));

    return 0;
}
static int
_cdqe(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE64, 0x98, -1, -1, ENC_NP, -1));

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
static int
_clc(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xf8, -1, -1, ENC_NP, -1));

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
static int
_cld(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xfc, -1, -1, ENC_NP, -1));

    return 0;
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
static int
_clflush(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x0f, 0xae, -1, ENC_M_M8, 7));

    return 0;
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
static int
_cli(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xfa, -1, -1, ENC_NP, -1));

    return 0;
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
_cmp(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x3c, -1, -1, ENC_I_AL_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x3d, -1, -1, ENC_I_AX_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x3d, -1, -1, ENC_I_EAX_IMM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x3d, -1, -1, ENC_I_RAX_IMM32, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x80, -1, -1, ENC_MI_RM8_IMM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x81, -1, -1, ENC_MI_RM16_IMM16, 7));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x81, -1, -1, ENC_MI_RM32_IMM32, 7));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x81, -1, -1, ENC_MI_RM64_IMM32, 7));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x83, -1, -1, ENC_MI_RM16_IMM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x83, -1, -1, ENC_MI_RM32_IMM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x83, -1, -1, ENC_MI_RM64_IMM8, 7));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x38, -1, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x39, -1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x39, -1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x39, -1, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x3a, -1, -1, ENC_RM_R8_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x3b, -1, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x3b, -1, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x3b, -1, -1, ENC_RM_R64_RM64, -1));

    return 0;
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
_cmpxchg(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x0f, 0xb0, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xb1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xb1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xb1, -1, ENC_MR_RM64_R64, -1));

    return 0;
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
static int
_cpuid(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0xa2, -1, ENC_NP, -1));

    return 0;
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
_crc32(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* FIXME */
    xstmt->prefix |= OPCODE_PREFIX_CRC32;

    EC(binstr2(asmblr, xstmt, SIZE8, 0x0f, 0x38, 0xf0, ENC_RM_R32_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0x38, 0xf1, ENC_RM_R32_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0x38, 0xf1, ENC_RM_R32_RM32, -1));

    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0x38, 0xf0, ENC_RM_R64_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0x38, 0xf1, ENC_RM_R64_RM64, -1));

    return 0;

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
static int
_cwd(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x99, -1, -1, ENC_NP, -1));

    return 0;
}
static int
_cdq(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE32, 0x99, -1, -1, ENC_NP, -1));

    return 0;
}
static int
_cqo(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE64, 0x99, -1, -1, ENC_NP, -1));

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
static int
_daa(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* FIXME: Invalidate in 64-bit mode */
    EC(binstr2(asmblr, xstmt, 0, 0x27, -1, -1, ENC_NP, -1));

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
static int
_das(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* FIXME: Invalidate in 64-bit mode */
    EC(binstr2(asmblr, xstmt, 0, 0x2f, -1, -1, ENC_NP, -1));

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
_dec(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xfe, -1, -1, ENC_M_RM8, 1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xff, -1, -1, ENC_M_RM16, 1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xff, -1, -1, ENC_M_RM32, 1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xff, -1, -1, ENC_M_RM64, 1));

    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE16, 0x48, -1, -1, ENC_O_R16, -1));*/
    /*EC(binstr2(asmblr, xstmt, SIZE32, 0x48, -1, -1, ENC_O_R32, -1));*/

    return 0;
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
_div(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xf6, -1, -1, ENC_M_RM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xf7, -1, -1, ENC_M_RM16, 6));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xf7, -1, -1, ENC_M_RM32, 6));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xf7, -1, -1, ENC_M_RM64, 6));

    return 0;
}

/*
 * HLT (Vol. 2A 3-368)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      F4              HLT                     NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
static int
_hlt(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xf4, -1, -1, ENC_NP, -1));

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
_idiv(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xf6, -1, -1, ENC_M_RM8, 7));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xf7, -1, -1, ENC_M_RM16, 7));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xf7, -1, -1, ENC_M_RM32, 7));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xf7, -1, -1, ENC_M_RM64, 7));

    return 0;
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
_imul(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xf6, -1, -1, ENC_M_RM8, 5));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xf7, -1, -1, ENC_M_RM16, 5));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xf7, -1, -1, ENC_M_RM32, 5));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xf7, -1, -1, ENC_M_RM64, 5));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xaf, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xaf, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xaf, -1, ENC_RM_R64_RM64, -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x6b, -1, -1, ENC_RMI_R16_RM16_IMM8,
               -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x6b, -1, -1, ENC_RMI_R32_RM32_IMM8,
               -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x6b, -1, -1, ENC_RMI_R64_RM64_IMM8,
               -1));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x69, -1, -1, ENC_RMI_R16_RM16_IMM16,
               -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x69, -1, -1, ENC_RMI_R32_RM32_IMM32,
               -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x69, -1, -1, ENC_RMI_R64_RM64_IMM32,
               -1));

    return 0;
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
_in(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xe4, -1, -1, ENC_I_AL_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xe5, -1, -1, ENC_I_AX_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xe5, -1, -1, ENC_I_EAX_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE8, 0xec, -1, -1, ENC_NP_AL_DX, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xed, -1, -1, ENC_NP_AX_DX, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xed, -1, -1, ENC_NP_EAX_DX, -1));

    return 0;
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
_inc(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xfe, -1, -1, ENC_M_RM8, 0));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xff, -1, -1, ENC_M_RM16, 0));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xff, -1, -1, ENC_M_RM32, 0));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xff, -1, -1, ENC_M_RM64, 0));

    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE16, 0x40, -1, -1, ENC_O_R16, -1));*/
    /*EC(binstr2(asmblr, xstmt, SIZE32, 0x40, -1, -1, ENC_O_R32, -1));*/

    return 0;
}

/*
 * INT (Vol. 2A 3-392)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      CC              INT 3                   NP      Valid   Valid
 *      CD              INT imm8                I       Valid   Valid
 *      CE              INTO                    NP      Inv.    Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 *      I       imm8            NA              NA              NA
 */
static int
_int(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xcc, -1, -1, ENC_NP_3, -1));
    EC(binstr2(asmblr, xstmt, 0, 0xcd, -1, -1, ENC_I_IMM8, -1));

    return 0;
}
static int
_into(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, 0, 0xce, -1, -1, ENC_NP, -1));*/

    return 0;
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
_iret(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0xcf, -1, -1, ENC_NP, 0));

    return 0;
}
static int
_iretd(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE32, 0xcf, -1, -1, ENC_NP, 0));

    return 0;
}
static int
_iretq(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE64, 0xcf, -1, -1, ENC_NP, 0));

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
static int
_jmp(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    if ( OPCODE_SUFFIX_FAR & xstmt->suffix ) {
        /* w/ far */

        /* To be implemented */
        return -EUNKNOWN;
    } else {
        /* w/o far */
        EC(binstr2(asmblr, xstmt, 0, 0xeb, -1, -1, ENC_D_REL8, -1));
        EC(binstr2(asmblr, xstmt, 0, 0xe9, -1, -1, ENC_D_REL32, -1));
        EC(binstr2(asmblr, xstmt, 0, 0xff, -1, -1, ENC_M_RM64, 4));
        /* Invalid for 64-bit mode */
        /*EC(binstr2(asmblr, xstmt, SIZE16, 0xe9, -1, -1, ENC_D_REL16, -1));*/
        /*EC(binstr2(asmblr, xstmt, SIZE16, 0xff, -1, -1, ENC_M_RM16, 4));*/
        /*EC(binstr2(asmblr, xstmt, SIZE32, 0xff, -1, -1, ENC_M_RM32, 4));*/

        return 0;
    }
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
static int
_monitor(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0x01, 0xc8, ENC_NP, -1));

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
 *                      MOV r/m64,imm32         MI      Valid   N.E.
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
static int
_mov(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x88, -1, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x89, -1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x89, -1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x89, -1, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x8a, -1, -1, ENC_RM_R8_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x8b, -1, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x8b, -1, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x8b, -1, -1, ENC_RM_R64_RM64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0xb0, -1, -1, ENC_OI_R8_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xb8, -1, -1, ENC_OI_R16_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xb8, -1, -1, ENC_OI_R32_IMM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xb8, -1, -1, ENC_OI_R64_IMM64, -1));

    /* FIXME: Support moffs */

    EC(binstr2(asmblr, xstmt, SIZE8, 0xc6, -1, -1, ENC_MI_RM8_IMM8, 0));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xc7, -1, -1, ENC_MI_RM16_IMM16, 0));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xc7, -1, -1, ENC_MI_RM32_IMM32, 0));
    EC(binstr2(asmblr, xstmt, SIZE64, 0xc7, -1, -1, ENC_MI_RM64_IMM32, 0));

    return 0;
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
_out(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0xe6, -1, -1, ENC_I_IMM8_AL, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xe7, -1, -1, ENC_I_IMM8_AX, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xe7, -1, -1, ENC_I_IMM8_EAX, -1));
    EC(binstr2(asmblr, xstmt, SIZE8, 0xee, -1, -1, ENC_NP_DX_AL, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0xef, -1, -1, ENC_NP_DX_AX, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0xef, -1, -1, ENC_NP_DX_EAX, -1));

    return 0;
}

/*
 * POP (Vol. 2B 4-263)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      8F /0           POP r/m16               M       Valid   Valid
 *      8F /0           POP r/m32               M       N.E.    Valid
 *      8F /0           POP r/m64               M       Valid   N.E.
 *      58+ rw          POP r16                 O       Valid   Valid
 *      58+ rd          POP r32                 O       N.E.    Valid
 *      58+ rd          POP r64                 O       Valid   N.E.
 *      1F              POP DS                  NP      Inv.    Valid
 *      07              POP ES                  NP      Inv.    Valid
 *      17              POP SS                  NP      Inv.    Valid
 *      0F A1           POP FS                  NP      Valid   Valid
 *      0F A1           POP FS                  NP      Inv.    Valid
 *      0F A1           POP FS                  NP      N.E.    Valid
 *      0F A9           POP GS                  NP      Valid   Valid
 *      0F A9           POP GS                  NP      Inv.    Valid
 *      0F A9           POP GS                  NP      N.E.    Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M(r)       NA              NA              NA
 *      O       opcode+rd(r,w)  NA              NA              NA
 *      NP      NA              NA              NA              NA
 */
static int
_pop(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0x8f, -1, -1, ENC_M_RM16, 0));
    EC(binstr2(asmblr, xstmt, 0, 0x8f, -1, -1, ENC_M_RM64, 0));

    //EC(binstr2(asmblr, xstmt, SIZE16, 0x58, -1, -1, ENC_O_R16, -1));
    //EC(binstr2(asmblr, xstmt, 0, 0x58, -1, -1, ENC_O_R64, -1));

    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0xa1, -1, ENC_NP_FS, -1));
    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0xa9, -1, ENC_NP_GS, -1));

    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, 0, 0x8f, -1, -1, ENC_M_RM32, 0));*/
    /*EC(binstr2(asmblr, xstmt, 0, 0x58, -1, -1, ENC_O_R32, -1));*/

    return 0;
}

/*
 * POPA/POPAD (Vol. 2B 4-268)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      61              POPA                    NP      Inv.    Valid
 *      61              POPAD                   NP      Inv.    Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
static int
_popa(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE16, 0x61, -1, -1, ENC_NP, -1));*/

    return 0;
}
static int
_popad(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE32, 0x61, -1, -1, ENC_NP, -1));*/

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
static int
_popcnt(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* FIXME */
    xstmt->prefix |= OPCODE_PREFIX_POPCNT;

    EC(binstr2(asmblr, xstmt, SIZE16, 0x0f, 0xb8, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x0f, 0xb8, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x0f, 0xb8, -1, ENC_RM_R64_RM64, -1));

    return 0;
}

/*
 * PUSH (Vol. 2B 4-333)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FF /6           PUSH r/m16              M       Valid   Valid
 *      FF /6           PUSH r/m32              M       N.E.    Valid
 *      FF /6           PUSH r/m64              M       Valid   N.E.
 *      50+ rw          PUSH r16                O       Valid   Valid
 *      50+ rd          PUSH r32                O       N.E.    Valid
 *      50+ rd          PUSH r64                O       Valid   N.E.
 *      6A ib           PUSH imm8               I       Valid   Valid
 *      6B iw           PUSH imm16              I       Valid   Valid
 *      6B id           PUSH imm32              I       Valid   Valid
 *      0E              PUSH CS                 NP      Inv.    Valid
 *      16              PUSH SS                 NP      Inv.    Valid
 *      1E              PUSH DS                 NP      Inv.    Valid
 *      06              PUSH ES                 NP      Inv.    Valid
 *      0F A0           PUSH FS                 NP      Valid   Valid
 *      0F A8           PUSH GS                 NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      M       ModR/M(r)       NA              NA              NA
 *      O       opcode+rd(r,w)  NA              NA              NA
 *      I       imm8/16/32      NA              NA              NA
 *      NP      NA              NA              NA              NA
 */
static int
_push(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE16, 0xff, -1, -1, ENC_M_RM16, 6));
    EC(binstr2(asmblr, xstmt, 0, 0xff, -1, -1, ENC_M_RM64, 6));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x50, -1, -1, ENC_O_R16, -1));
    EC(binstr2(asmblr, xstmt, 0, 0x50, -1, -1, ENC_O_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x6a, -1, -1, ENC_I_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x6b, -1, -1, ENC_I_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x6b, -1, -1, ENC_I_IMM32, -1));

    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0xa0, -1, ENC_NP_FS, -1));
    EC(binstr2(asmblr, xstmt, 0, 0x0f, 0xa8, -1, ENC_NP_GS, -1));

    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, 0, 0xff, -1, -1, ENC_M_RM32, 6));*/
    /*EC(binstr2(asmblr, xstmt, 0, 0x50, -1, -1, ENC_O_R32, -1));*/

    return 0;
}

/*
 * PUSHA/PUSHAD (Vol. 2B 4-337)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      60              PUSHA                   NP      Inv.    Valid
 *      60              PUSHAD                  NP      Inv.    Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
static int
_pusha(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE16, 0x60, -1, -1, ENC_NP, -1));*/

    return 0;
}
static int
_pushad(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    /* Invalid for 64-bit mode */
    /*EC(binstr2(asmblr, xstmt, SIZE32, 0x60, -1, -1, ENC_NP, -1));*/

    return 0;
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
_ret(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    if ( OPCODE_SUFFIX_FAR & xstmt->suffix ) {
        /* w/ far */
        EC(binstr2(asmblr, xstmt, 0, 0xcb, -1, -1, ENC_NP, -1));
        EC(binstr2(asmblr, xstmt, 0, 0xca, -1, -1, ENC_I_IMM16, -1));
    } else {
        /* w/o far */
        EC(binstr2(asmblr, xstmt, 0, 0xc3, -1, -1, ENC_NP, -1));
        EC(binstr2(asmblr, xstmt, 0, 0xc2, -1, -1, ENC_I_IMM16, -1));
    }

    return 0;
}

/*
 * STI (Vol. 2B 4-441)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      FB              STI                     NP      Valid   Valid
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      NP      NA              NA              NA              NA
 */
static int
_sti(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, 0, 0xfb, -1, -1, ENC_NP, -1));

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
static int
_xor(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    EC(binstr2(asmblr, xstmt, SIZE8, 0x34, -1, -1, ENC_I_AL_IMM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x35, -1, -1, ENC_I_AX_IMM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x35, -1, -1, ENC_I_EAX_IMM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x35, -1, -1, ENC_I_RAX_IMM32, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x80, -1, -1, ENC_MI_RM8_IMM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x81, -1, -1, ENC_MI_RM16_IMM16, 6));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x81, -1, -1, ENC_MI_RM32_IMM32, 6));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x81, -1, -1, ENC_MI_RM64_IMM32, 6));

    EC(binstr2(asmblr, xstmt, SIZE16, 0x83, -1, -1, ENC_MI_RM16_IMM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x83, -1, -1, ENC_MI_RM32_IMM8, 6));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x83, -1, -1, ENC_MI_RM64_IMM8, 6));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x30, -1, -1, ENC_MR_RM8_R8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x31, -1, -1, ENC_MR_RM16_R16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x31, -1, -1, ENC_MR_RM32_R32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x31, -1, -1, ENC_MR_RM64_R64, -1));

    EC(binstr2(asmblr, xstmt, SIZE8, 0x32, -1, -1, ENC_RM_R8_RM8, -1));
    EC(binstr2(asmblr, xstmt, SIZE16, 0x33, -1, -1, ENC_RM_R16_RM16, -1));
    EC(binstr2(asmblr, xstmt, SIZE32, 0x33, -1, -1, ENC_RM_R32_RM32, -1));
    EC(binstr2(asmblr, xstmt, SIZE64, 0x33, -1, -1, ENC_RM_R64_RM64, -1));

    return 0;
}


/*
 * A macro to register an assembler function into this file
 */
#define REGISTER_INSTR(f, str, x) do {          \
        if ( 0 == strcasecmp(#x, (str)) ) {     \
            (f) = _##x;                         \
        }                                       \
    } while ( 0 )

/*
 * Get the function pointer w/ instruction prefixes corresponding to the opcode
 */
static int
_resolv_instr(x86_64_stmt_t *xstmt)
{
    x86_64_instr_f ifunc;
    char *str;
    int i;
    int tmpprefix;
    int tmpsuffix;

    ifunc = NULL;

    if ( mvector_size(xstmt->stmt->u.instr->opcode) < 1 ) {
        /* No opcode found */
        return -1;
    }

    /* Parse prefixes */
    tmpprefix = 0;
    for ( i = 0; i < mvector_size(xstmt->stmt->u.instr->opcode); i++ ) {
        str = mvector_at(xstmt->stmt->u.instr->opcode, i);
        if ( 0 == strcasecmp(str, "lock") ) {
            tmpprefix |= OPCODE_PREFIX_LOCK;
        } else if ( 0 == strcasecmp(str, "rep") ) {
            tmpprefix |= OPCODE_PREFIX_REP;
        } else if ( 0 == strcasecmp(str, "repe") ) {
            tmpprefix |= OPCODE_PREFIX_REPE;
        } else if ( 0 == strcasecmp(str, "repz") ) {
            tmpprefix |= OPCODE_PREFIX_REPZ;
        } else if ( 0 == strcasecmp(str, "repne") ) {
            tmpprefix |= OPCODE_PREFIX_REPNE;
        } else if ( 0 == strcasecmp(str, "repnz") ) {
            tmpprefix |= OPCODE_PREFIX_REPNZ;
        } else if ( 0 == strcasecmp(str, "brt") ) {
            tmpprefix |= OPCODE_PREFIX_BRANCH_TAKEN;
        } else if ( 0 == strcasecmp(str, "brn") ) {
            tmpprefix |= OPCODE_PREFIX_BRANCH_NOT_TAKEN;
        } else {
            break;
        }
    }

   /* Opcode */
   if ( i < mvector_size(xstmt->stmt->u.instr->opcode) ) {
       str = mvector_at(xstmt->stmt->u.instr->opcode, i);
       REGISTER_INSTR(ifunc, str, add);
       REGISTER_INSTR(ifunc, str, and);
       REGISTER_INSTR(ifunc, str, bsf);
       REGISTER_INSTR(ifunc, str, bsr);
       REGISTER_INSTR(ifunc, str, bswap);
       REGISTER_INSTR(ifunc, str, bt);
       REGISTER_INSTR(ifunc, str, btc);
       REGISTER_INSTR(ifunc, str, btr);
       REGISTER_INSTR(ifunc, str, bts);
       REGISTER_INSTR(ifunc, str, call);
       REGISTER_INSTR(ifunc, str, cbw);
       REGISTER_INSTR(ifunc, str, cwde);
       REGISTER_INSTR(ifunc, str, cdqe);
       REGISTER_INSTR(ifunc, str, clc);
       REGISTER_INSTR(ifunc, str, cld);
       REGISTER_INSTR(ifunc, str, clflush);
       REGISTER_INSTR(ifunc, str, cli);
       REGISTER_INSTR(ifunc, str, cmp);
       REGISTER_INSTR(ifunc, str, cmpxchg);
       REGISTER_INSTR(ifunc, str, cpuid);
       REGISTER_INSTR(ifunc, str, crc32);
       REGISTER_INSTR(ifunc, str, cwd);
       REGISTER_INSTR(ifunc, str, cdq);
       REGISTER_INSTR(ifunc, str, cqo);
       REGISTER_INSTR(ifunc, str, daa);
       REGISTER_INSTR(ifunc, str, das);
       REGISTER_INSTR(ifunc, str, dec);
       REGISTER_INSTR(ifunc, str, div);
       REGISTER_INSTR(ifunc, str, hlt);
       REGISTER_INSTR(ifunc, str, idiv);
       REGISTER_INSTR(ifunc, str, imul);
       REGISTER_INSTR(ifunc, str, in);
       REGISTER_INSTR(ifunc, str, inc);
       REGISTER_INSTR(ifunc, str, int);
       REGISTER_INSTR(ifunc, str, into);
       REGISTER_INSTR(ifunc, str, iret);
       REGISTER_INSTR(ifunc, str, iretd);
       REGISTER_INSTR(ifunc, str, iretq);
       REGISTER_INSTR(ifunc, str, jmp);
       REGISTER_INSTR(ifunc, str, monitor);
       REGISTER_INSTR(ifunc, str, mov);
       REGISTER_INSTR(ifunc, str, out);
       REGISTER_INSTR(ifunc, str, pop);
       REGISTER_INSTR(ifunc, str, popa);
       REGISTER_INSTR(ifunc, str, popad);
       REGISTER_INSTR(ifunc, str, popcnt);
       REGISTER_INSTR(ifunc, str, push);
       REGISTER_INSTR(ifunc, str, pusha);
       REGISTER_INSTR(ifunc, str, pushad);
       REGISTER_INSTR(ifunc, str, ret);
       REGISTER_INSTR(ifunc, str, sti);
       REGISTER_INSTR(ifunc, str, xor);
   } else {
       return -1;
   }
   i++;

   /* Check if the function is found */
   if ( NULL == ifunc ) {
       return -1;
   }

    /* Parse suffixes */
    tmpsuffix = 0;
    for ( ; i < mvector_size(xstmt->stmt->u.instr->opcode); i++ ) {
        str = mvector_at(xstmt->stmt->u.instr->opcode, i);
        if ( 0 == strcasecmp(str, "far") ) {
            tmpsuffix |= OPCODE_SUFFIX_FAR;
        } else {
            break;
        }
    }

    if ( i != mvector_size(xstmt->stmt->u.instr->opcode) ) {
        return -1;
    }

    /* Set the values */
    xstmt->prefix = tmpprefix;
    xstmt->suffix = tmpsuffix;
    xstmt->ifunc = ifunc;

    return 0;
}

static int
_check_size(size_t sz, int64_t val)
{
    switch ( sz ) {
    case SIZE8:
        if ( val >= -128 && val <= 127 ) {
            return 1;
        }
        break;
    case SIZE16:
        if ( val >= -32768 && val <= 32767 ) {
            return 1;
        }
        break;
    case SIZE32:
        if ( val >= -2147483648 && val <= 2147483647 ) {
            return 1;
        }
        break;
    case SIZE64:
        return 1;
    default:
        if ( 0 == val ) {
            return 1;
        }
    }

    return 0;
}

/*
 * Fix instructions
 */
static int
_fix_instr(x86_64_stmt_t *xstmt)
{
    int i;
    size_t imin;
    size_t imax;
    size_t sz;
    x86_64_instr_t *instr;
    x86_64_instr_t *minstr;
    int fixed;

    /* Fix the optimal instruction if possible */
    imin = 0;
    imax = 0;
    instr = NULL;
    minstr = NULL;
    fixed = 1;
    for ( i = 0; i < mvector_size(xstmt->instrs); i++ ) {
        instr = mvector_at(xstmt->instrs, i);
        if ( NULL != instr->disp.expr || NULL != instr->imm.expr
             || 0 != instr->rel.sz ) {
            /* Not fixed */
            fixed = 0;
        }

        /* Check the operands whether they are acceptable values */
        if ( !_check_size(instr->disp.sz, instr->disp.val)
             || !_check_size(instr->imm.sz, instr->imm.val)
             || !_check_size(instr->rel.sz, instr->rel.val) ) {
            continue;
        }

        sz = _instr_size(instr);
        if ( 0 == imin || sz < imin ) {
            /* Update minimum */
            imin = sz;
            minstr = instr;
        }
        if ( sz > imax ) {
            imax = sz;
        }
    }
    if ( 0 == imin ) {
        return -EOPERAND;
    }

    if ( fixed ) {
        /* Fixed */
        xstmt->sinstr = minstr;
        xstmt->esize.min = imin;
        xstmt->esize.max = imin;
    } else {
        /* Not fixed yet */
        xstmt->esize.min = imin;
        xstmt->esize.max = imax;
    }

    return 0;
}

/*
 * Fix instructions
 */
static int
_fix_instr2(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    int i;
    size_t imin;
    size_t imax;
    int64_t loff;
    int64_t roff;
    size_t sz;
    x86_64_instr_t *instr;
    x86_64_instr_t *minstr;
    int ret;

    /* Fix the optimal instruction if possible */
    imin = 0;
    imax = 0;
    instr = NULL;
    minstr = NULL;
    for ( i = 0; i < mvector_size(xstmt->instrs); i++ ) {
        /* Check if the optimal instruction is fixed */
        if ( NULL != xstmt->sinstr ) {
            /* Already fixed */
            continue;
        }

        /* Obtain a candidate instruction */
        instr = mvector_at(xstmt->instrs, i);

        /* Obtain the size of the instruction */
        sz = _instr_size(instr);

        /* Check whether they are capable values */
        if ( NULL != instr->disp.expr ) {
            /* Displacement is relocatable */
            ret = x86_64_expr_range(&asmblr->lbtbl, instr->disp.expr, &loff,
                                    &roff);
            if ( ret < 0 ) {
                return -1;
            }
            if ( !_check_size(instr->disp.sz, loff)
                 || !_check_size(instr->disp.sz, roff) ) {
                continue;
            }
        }
        if ( NULL != instr->imm.expr ) {
            /* Immediate value is relocatable */
            ret = x86_64_expr_range(&asmblr->lbtbl, instr->imm.expr, &loff,
                                    &roff);
            if ( ret < 0 ) {
                return -1;
            }
            if ( !_check_size(instr->imm.sz, loff)
                 || !_check_size(instr->imm.sz, roff) ) {
                continue;
            }
        }
        if ( 0 != instr->rel.sz ) {
            /* Relative value */
            if ( NULL == instr->rel.expr ) {
                //loff = instr->rel.val - xstmt->epos.max - sz;
                //roff = instr->rel.val - xstmt->epos.min - sz;
                loff = X86_64_VAR_MIN;
                roff = X86_64_VAR_MAX;
                if ( !_check_size(instr->rel.sz, loff)
                     || !_check_size(instr->rel.sz, roff) ) {
                    continue;
                }
            } else {
                ret = x86_64_expr_range(&asmblr->lbtbl, instr->rel.expr, &loff,
                                        &roff);
                if ( ret < 0 ) {
                    return -1;
                }
                loff = loff - xstmt->epos.max - sz;
                roff = roff - xstmt->epos.min - sz;
                if ( !_check_size(instr->rel.sz, loff)
                     || !_check_size(instr->rel.sz, roff) ) {
                    continue;
                }
            }
        }

        if ( 0 == imin || sz < imin ) {
            /* Update minimum */
            imin = sz;
            minstr = instr;
        }
        if ( sz > imax ) {
            imax = sz;
        }
    }
    if ( 0 == imin ) {
        return -EOPERAND;
    }

    /* Fix */
    xstmt->sinstr = minstr;
    xstmt->esize.min = imin;
    xstmt->esize.max = imin;

    return 0;
}

/*
 * Fix instructions
 */
static int
_fix_instr3(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    x86_64_instr_t *instr;
    x86_64_rval_t *rval;
    x86_64_rela_t *rela;
    size_t sz;

    /* Selected instruction */
    instr = xstmt->sinstr;
    if ( NULL == instr ) {
        return -1;
    }


    /* Obtain the size of the instruction */
    sz = _instr_size(instr);

    /* Check whether they are capable values */
    if ( NULL != instr->disp.expr ) {
        /* Displacement is relocatable */
        rval = x86_64_expr_fix(&asmblr->lbtbl, instr->disp.expr);
        if ( NULL == rval ) {
            return -1;
        }
        fprintf(stderr, "DEBUG: %d %s %lld\n", rval->nsym,
                rval->symname ? rval->symname : "-",
                rval->addend);
        /* Free */
        if ( NULL != rval->symname ) {
            free(rval->symname);
        }
        free(rval);
    }
    if ( NULL != instr->imm.expr ) {
        /* Immediate value is relocatable */
        rval = x86_64_expr_fix(&asmblr->lbtbl, instr->imm.expr);
        if ( NULL == rval ) {
            return -1;
        }
        fprintf(stderr, "DEBUG: %d %s %lld\n", rval->nsym,
                rval->symname ? rval->symname : "-",
                rval->addend);
        /* Free */
        if ( NULL != rval->symname ) {
            free(rval->symname);
        }
        free(rval);
    }
    if ( 0 != instr->rel.sz ) {
        /* Relative value */
        if ( NULL == instr->rel.expr ) {
            rela = malloc(sizeof(x86_64_rela_t));
            if ( NULL == rela ) {
                return -1;
            }
            /* FIXME: Replace with the fixed position */
            rela->offset = xstmt->epos.min + sz - instr->rel.sz;
            rela->type = X86_64_RELA_PC32; /* FIXME */
            rela->addend = instr->rel.val;
            /* S = 0 */
            rela->symname = NULL;

            /* Reset the value */
            instr->rel.val = 0;
        } else {
            rval = x86_64_expr_fix(&asmblr->lbtbl, instr->rel.expr);
            if ( NULL == rval ) {
                return -1;
            }
            fprintf(stderr, "DEBUG: %d %s %lld\n", rval->nsym,
                    rval->symname ? rval->symname : "-",
                    rval->addend);

            /* To relocatable value */
            rela = malloc(sizeof(x86_64_rela_t));
            if ( NULL == rela ) {
                if ( NULL != rval->symname ) {
                    free(rval->symname);
                }
                free(rval);
                return -1;
            }
            if ( 1 == rval->nsym ) {
                /* FIXME: Replace with the fixed position */
                rela->offset = xstmt->epos.min + sz - instr->rel.sz;
                rela->type = X86_64_RELA_PC32; /* FIXME */
                rela->addend = rval->addend - (xstmt->epos.min + sz);
                /* S = 0 */
                if ( NULL != rval->symname ) {
                    rela->symname = strdup(rval->symname);
                    if ( NULL == rela->symname ) {
                        free(rval->symname);
                        free(rval);
                        return -1;
                    }
                    /* Reset the value */
                    instr->rel.val = 0;

                } else {
                    free(rela);
                    /* Reset the value */
                    instr->rel.val = rval->addend - (xstmt->epos.min + sz);
                }
            } else {
                /* FIXME */
                if ( NULL != rval->symname ) {
                    free(rval->symname);
                }
                free(rval);
                return -1;
            }

            /* Free */
            if ( NULL != rval->symname ) {
                free(rval->symname);
            }
            free(rval);
        }
    }

    return 0;
}

/*
 * Assemble an instruction
 */
static int
_assemble_instr(x86_64_assembler_t *asmblr, x86_64_stmt_t *xstmt)
{
    int ret;
    int j;

    assert( STMT_INSTR == xstmt->stmt->type );

    /* Execute an assembling function */
    ret = xstmt->ifunc(asmblr, xstmt);
    if ( ret >= 0 ) {
        if ( 0 == mvector_size(xstmt->instrs) ) {
            /* Error */
            fprintf(stderr, "Error:");
            for ( j = 0; j < mvector_size(xstmt->stmt->u.instr->opcode); j++ ) {
                fprintf(stderr, " %s",
                        mvector_at(xstmt->stmt->u.instr->opcode, j));
            }
            fprintf(stderr, "\n");
            /* FIXME */
            return 0;
        } else {
            if ( 0 != _fix_instr(xstmt) ) {
                /* FIXME: Error */
            }
            return 0;
        }
    } else {
        /* Error */
        fprintf(stderr, "Error:");
        for ( j = 0; j < mvector_size(xstmt->stmt->u.instr->opcode); j++ ) {
            fprintf(stderr, " %s",
                    mvector_at(xstmt->stmt->u.instr->opcode, j));
        }
        fprintf(stderr, "\n");
        /* FIXME */
        return 0;
    }

    return 0;
}


/*
 * Add label to the table
 */
static int
_add_label(x86_64_label_table_t *tbl, const char *lstr, off_t imin, off_t imax)
{
    x86_64_label_t *lb;
    x86_64_label_ent_t **ent;

    ent = &(tbl->root);
    while ( NULL != *ent ) {
        if ( 0 == strcmp((*ent)->c->label, lstr) ) {
            /* Duplicated */
            return -EDUP;
        }
        ent = &((*ent)->next);
    }
    *ent = malloc(sizeof(x86_64_label_ent_t));
    if ( NULL == *ent ) {
        return -EGENERIC;
    }

    /* Allocate memory for a label structure */
    lb = malloc(sizeof(x86_64_label_t));
    if ( NULL == lb ) {
        free(*ent);
        return -EGENERIC;
    }
    lb->label = strdup(lstr);
    if ( NULL == lb->label ) {
        free(*ent);
        free(lb);
        return -EGENERIC;
    }
    /* Set the expected positions of this instruction */
    lb->min = imin;
    lb->max = imax;
    lb->scope = 0;

    /* Set */
    (*ent)->c = lb;
    (*ent)->next = NULL;

    return 0;
}

/*
 * Clear the label table
 * Note that this does not free the label table structure
 */
static void
_label_table_clear(x86_64_label_table_t *tbl)
{
    x86_64_label_ent_t *ent;
    x86_64_label_ent_t *tmpent;

    ent = tbl->root;
    while ( NULL != ent ) {
        tmpent = ent->next;
        free(ent->c->label);
        free(ent->c);
        free(ent);
        ent = tmpent;
    }
    tbl->root = NULL;
}

/*
 * Set a label as global scope
 */
static int
_set_label_global(x86_64_label_table_t *tbl, const char *lstr)
{
    x86_64_label_ent_t *ent;

    ent = tbl->root;
    while ( NULL != ent ) {
        if ( 0 == strcmp(ent->c->label, lstr) ) {
            /* Set the scope as global */
            ent->c->scope |= 1;
            return 0;
        }
        ent = ent->next;
    }

    return -1;
}

/*
 * Fixed the label position
 */
static int
_fix_label_position(x86_64_label_table_t *tbl, const char *lstr, off_t pos)
{
    x86_64_label_ent_t *ent;

    ent = tbl->root;
    while ( NULL != ent ) {
        if ( 0 == strcmp(ent->c->label, lstr) ) {
            ent->c->min = pos;
            ent->c->max = pos;
            return 0;
        }
        ent = ent->next;
    }

    return -1;
}


/*
 * Stage 1: Assemble all the candidates
 * State 2: Estimate label offsets
 * Stage 3: Complete the assembling procedure
 */
static int
_stage1(x86_64_assembler_t *asmblr, const stmt_vector_t *vec)
{
    size_t i;
    size_t j;
    stmt_t *stmt;
    int ret;
    off_t pos;
    x86_64_stmt_t *xstmt;
    x86_64_stmt_vector_t *xvec;
    x86_64_target_t tgt;

    assert( 0 == asmblr->stage );

    /* Allocate a new statement vector */
    xvec = mvector_new();

    /* Set the default target */
    tgt = X86_64_O64;

    /* Estimate the position of labels */
    pos = 0;
    for ( i = 0; i < mvector_size(vec); i++ ) {
        stmt = mvector_at(vec, i);

        xstmt = malloc(sizeof(x86_64_stmt_t));
        if ( NULL == xstmt ) {
            /* FIXME */
            mvector_delete(xvec);
            return -1;
        }
        xstmt->stmt = stmt;
        xstmt->state = X86_64_STMT_INIT;
        xstmt->tgt = tgt;
        xstmt->evals = NULL;
        xstmt->sinstr = NULL;
        xstmt->instrs = mvector_new();
        xstmt->esize.min = 0;
        xstmt->esize.max = 0;

        /* Append */
        if ( NULL == mvector_push_back(xvec, xstmt) ) {
            /* FIXME: Must free the contents of the vector */
            mvector_delete(xvec);
            return -1;
        }

        switch ( stmt->type ) {
        case STMT_INSTR:
            /* Resolve the corresponding function pointer to the assembling
               this instruction */
            ret = _resolv_instr(xstmt);
            if ( 0 != ret ) {
                /* Unknown */
                fprintf(stderr, "Unknown instruction:");
                for ( j = 0; j < mvector_size(xstmt->stmt->u.instr->opcode);
                      j++ ) {
                    fprintf(stderr, " %s",
                            mvector_at(xstmt->stmt->u.instr->opcode, j));
                }
                fprintf(stderr, "\n");
                /* FIXME: Must free the contents of the vector */
                mvector_delete(xvec);
                return -1;
            }

            /* Assemble the instruction */
            ret = _assemble_instr(asmblr, xstmt);
            if ( 0 != ret ) {
                /* Error on assembling the instruction */
                /* FIXME: Must free the contents of the vector */
                mvector_delete(xvec);
                return -1;
            }
            pos++;
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    asmblr->xvec = xvec;
    asmblr->stage = 1;

    return 0;
}
static int
_stage2(x86_64_assembler_t *asmblr)
{
    size_t i;
    x86_64_stmt_t *xstmt;
    int ret;
    off_t imin;
    off_t imax;

    assert( 1 == asmblr->stage );

    imin = 0;
    imax = 0;

    for ( i = 0; i < mvector_size(asmblr->xvec); i++ ) {
        /* Get a statement at i-th position */
        xstmt = mvector_at(asmblr->xvec, i);

        /* Set the estimated position */
        xstmt->epos.min = imin;
        xstmt->epos.max = imax;

        switch ( xstmt->stmt->type ) {
        case STMT_INSTR:
            /* Estimate the label position */
            imin += xstmt->esize.min;
            imax += xstmt->esize.max;
            break;
        case STMT_LABEL:
            /* Add it to the label table */
            ret = _add_label(&asmblr->lbtbl, xstmt->stmt->u.label, imin, imax);
            if ( ret < 0 ) {
                if ( -EDUP == ret ) {
                    fprintf(stderr, "A duplicate label: %s\n",
                            xstmt->stmt->u.label);
                }
                _label_table_clear(&asmblr->lbtbl);
                return -1;
            }

            break;
        default:
            /* Do nothing */
            ;
        }
    }

    /* Finishing stage 2 */
    asmblr->stage = 2;

    return 0;
}

static int
_stage3(x86_64_assembler_t *asmblr)
{
    size_t i;
    x86_64_stmt_t *xstmt;
    off_t pos;

    assert( 2 == asmblr->stage );

    for ( i = 0; i < mvector_size(asmblr->xvec); i++ ) {
        xstmt = mvector_at(asmblr->xvec, i);

        switch ( xstmt->stmt->type ) {
        case STMT_INSTR:
            _fix_instr2(asmblr, xstmt);
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    /* Fix the position */
    pos = 0;
    for ( i = 0; i < mvector_size(asmblr->xvec); i++ ) {
        xstmt = mvector_at(asmblr->xvec, i);

        switch ( xstmt->stmt->type ) {
        case STMT_INSTR:
            xstmt->epos.min = pos;
            xstmt->epos.max = pos;
            pos += xstmt->esize.min;
            break;
        case STMT_LABEL:
            /* Update the position */
            _fix_label_position(&asmblr->lbtbl, xstmt->stmt->u.label, pos);
            /*fprintf(stderr, "Label %s: %lld\n", xstmt->stmt->u.label, pos);*/
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    for ( i = 0; i < mvector_size(asmblr->xvec); i++ ) {
        xstmt = mvector_at(asmblr->xvec, i);

        switch ( xstmt->stmt->type ) {
        case STMT_INSTR:
            _fix_instr3(asmblr, xstmt);
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    /* Completed stage 3 */
    asmblr->stage = 3;

    return 0;
}

static int
_output(x86_64_assembler_t *asmblr)
{
    size_t i;
    x86_64_stmt_t *xstmt;

    assert( 3 == asmblr->stage );

    for ( i = 0; i < mvector_size(asmblr->xvec); i++ ) {
        xstmt = mvector_at(asmblr->xvec, i);

        switch ( xstmt->stmt->type ) {
        case STMT_INSTR:
            if ( NULL != xstmt->sinstr ) {
                _print_instruction_bin(xstmt->sinstr);
            } else {
                fprintf(stderr, "Not fixed instruction: # = %zu\n",
                        mvector_size(xstmt->instrs));
            }

#if 0
            fprintf(stderr, "# = %zu\n", mvector_size(xstmt->instrs));
            if ( 1 == mvector_size(xstmt->instrs) ) {
                _print_instruction_bin(mvector_at(xstmt->instrs, 0));
#if 0
                _print_instruction(instr);
                printf("\n");
#endif
            }
#endif
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    return 0;
}


/*
 * Assemble x86-64 code
 */
static int
_assemble(x86_64_assembler_t *asmblr, stmt_vector_t *vec)
{
    int ret;
    stmt_t *stmt;
    size_t i;

    /* Stage 1 */
    ret = _stage1(asmblr, vec);
    if ( ret < 0 ) {
        return -1;
    }

    /* Stage 2 */
    ret = _stage2(asmblr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Stage 3 */
    ret = _stage3(asmblr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Output */
    ret = _output(asmblr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Linker stage: Check global */
    for ( i = 0; i < mvector_size(vec); i++ ) {
        stmt = mvector_at(vec, i);
        switch ( stmt->type ) {
        case STMT_GLOBAL:
            ret = _set_label_global(&asmblr->lbtbl, stmt->u.global);
            if ( ret < 0 ) {
                fprintf(stderr, "Label %s is not defined\n", stmt->u.label);
                /* Free the label table */
                _label_table_clear(&asmblr->lbtbl);
                return -1;
            }
            break;
        default:
            /* Do nothing */
            ;
        }
    }

    return 0;
}

/*
 * Assembler code
 */
int
arch_x86_64_assemble(const char *fname)
{
    pcode_t *pcode;
    stmt_vector_t *vec;
    preprocessor_t *pp;
    int ret;
    x86_64_assembler_t asmblr;

    /* Create a preprocessor instance */
    pp = pp_new();

    /* Register *fixes */
    ret = pp_register_fix(pp, "lock");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "rep");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "repe");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "repz");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "repne");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "repnz");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }
    ret = pp_register_fix(pp, "far");
    if ( ret < 0 ) {
        pp_delete(pp);
        return -1;
    }

    /* Preprocess the code */
    pcode = preprocess(pp, fname);
    pp_delete(pp);
    if ( NULL == pcode ) {
        return -1;
    }

    /* Parse the code */
    vec = parse(pcode);
    if ( NULL == vec ) {
        return -1;
    }

    /* Initialize the label table */
    asmblr.lbtbl.root = NULL;
    asmblr.stage = 0;

    _assemble(&asmblr, vec);

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

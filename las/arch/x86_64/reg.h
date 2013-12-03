/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _ARCH_X86_64_REG_H
#define _ARCH_X86_64_REG_H

/*
 * x86-64 registers
 */
typedef enum op_reg {
    REG_UNKNOWN = -1,
    /* IP */
    REG_EIP,
    REG_RIP,
    /* AX */
    REG_AL = 0,
    REG_AH,
    REG_AX,
    REG_EAX,
    REG_RAX,
    /* CX */
    REG_CL,
    REG_CH,
    REG_CX,
    REG_ECX,
    REG_RCX,
    /* DX */
    REG_DL,
    REG_DH,
    REG_DX,
    REG_EDX,
    REG_RDX,
    /* BX */
    REG_BL,
    REG_BH,
    REG_BX,
    REG_EBX,
    REG_RBX,
    /* SP */
    REG_SPL,
    REG_SP,
    REG_ESP,
    REG_RSP,
    /* BP */
    REG_BPL,
    REG_BP,
    REG_EBP,
    REG_RBP,
    /* SI */
    REG_SIL,
    REG_SI,
    REG_ESI,
    REG_RSI,
    /* DI */
    REG_DIL,
    REG_DI,
    REG_EDI,
    REG_RDI,
    /* R8 */
    REG_R8L,
    REG_R8W,
    REG_R8D,
    REG_R8,
    /* R9 */
    REG_R9L,
    REG_R9W,
    REG_R9D,
    REG_R9,
    /* R10 */
    REG_R10L,
    REG_R10W,
    REG_R10D,
    REG_R10,
    /* R11 */
    REG_R11L,
    REG_R11W,
    REG_R11D,
    REG_R11,
    /* R12 */
    REG_R12L,
    REG_R12W,
    REG_R12D,
    REG_R12,
    /* R13 */
    REG_R13L,
    REG_R13W,
    REG_R13D,
    REG_R13,
    /* R14 */
    REG_R14L,
    REG_R14W,
    REG_R14D,
    REG_R14,
    /* R15 */
    REG_R15L,
    REG_R15W,
    REG_R15D,
    REG_R15,
    /* Segment registers */
    REG_CS,
    REG_DS,
    REG_ES,
    REG_FS,
    REG_GS,
    /* Flags */
    REG_FLAGS,
    REG_EFLAGS,
    REG_RFLAGS,
    /* MM */
    REG_MM0,
    REG_MM1,
    REG_MM2,
    REG_MM3,
    REG_MM4,
    REG_MM5,
    REG_MM6,
    REG_MM7,
    /* XMM */
    REG_XMM0,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7,
} x86_64_reg_t;

#ifdef __cplusplus
extern "C" {
#endif

    x86_64_reg_t strtoreg(const char *);
    int regsize(x86_64_reg_t);

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_X86_64_REG_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

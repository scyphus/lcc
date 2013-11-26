/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "reg.h"
#include <string.h>

/*
 * Convert the register string to x86_64_reg_t
 */
x86_64_reg_t
strtoreg(const char *s)
{
    x86_64_reg_t reg;

    if ( 0 == strcmp("al", s) ) {
        reg = REG_AL;
    } else if ( 0 == strcmp("ah", s) ) {
        reg = REG_AH;
    } else if ( 0 == strcmp("ax", s) ) {
        reg = REG_AX;
    } else if ( 0 == strcmp("eax", s) ) {
        reg = REG_EAX;
    } else if ( 0 == strcmp("rax", s) ) {
        reg = REG_RAX;
    } else if ( 0 == strcmp("cl", s) ) {
        reg = REG_CL;
    } else if ( 0 == strcmp("ch", s) ) {
        reg = REG_CH;
    } else if ( 0 == strcmp("cx", s) ) {
        reg = REG_CX;
    } else if ( 0 == strcmp("ecx", s) ) {
        reg = REG_ECX;
    } else if ( 0 == strcmp("rcx", s) ) {
        reg = REG_RCX;
    } else if ( 0 == strcmp("dl", s) ) {
        reg = REG_DL;
    } else if ( 0 == strcmp("dh", s) ) {
        reg = REG_DH;
    } else if ( 0 == strcmp("dx", s) ) {
        reg = REG_DX;
    } else if ( 0 == strcmp("edx", s) ) {
        reg = REG_EDX;
    } else if ( 0 == strcmp("rdx", s) ) {
        reg = REG_RDX;
    } else if ( 0 == strcmp("bl", s) ) {
        reg = REG_BL;
    } else if ( 0 == strcmp("bh", s) ) {
        reg = REG_BH;
    } else if ( 0 == strcmp("bx", s) ) {
        reg = REG_BX;
    } else if ( 0 == strcmp("ebx", s) ) {
        reg = REG_EBX;
    } else if ( 0 == strcmp("rbx", s) ) {
        reg = REG_RBX;
    } else if ( 0 == strcmp("spl", s) ) {
        reg = REG_SPL;
    } else if ( 0 == strcmp("sp", s) ) {
        reg = REG_SP;
    } else if ( 0 == strcmp("esp", s) ) {
        reg = REG_ESP;
    } else if ( 0 == strcmp("rsp", s) ) {
        reg = REG_RSP;
    } else if ( 0 == strcmp("bpl", s) ) {
        reg = REG_BPL;
    } else if ( 0 == strcmp("bp", s) ) {
        reg = REG_BP;
    } else if ( 0 == strcmp("ebp", s) ) {
        reg = REG_EBP;
    } else if ( 0 == strcmp("rbp", s) ) {
        reg = REG_RBP;
    } else if ( 0 == strcmp("sil", s) ) {
        reg = REG_SIL;
    } else if ( 0 == strcmp("si", s) ) {
        reg = REG_SI;
    } else if ( 0 == strcmp("esi", s) ) {
        reg = REG_ESI;
    } else if ( 0 == strcmp("rsi", s) ) {
        reg = REG_RSI;
    } else if ( 0 == strcmp("dil", s) ) {
        reg = REG_DIL;
    } else if ( 0 == strcmp("di", s) ) {
        reg = REG_DI;
    } else if ( 0 == strcmp("edi", s) ) {
        reg = REG_EDI;
    } else if ( 0 == strcmp("rdi", s) ) {
        reg = REG_RDI;
    } else if ( 0 == strcmp("r8l", s) ) {
        reg = REG_R8L;
    } else if ( 0 == strcmp("r8w", s) ) {
        reg = REG_R8W;
    } else if ( 0 == strcmp("r8d", s) ) {
        reg = REG_R8D;
    } else if ( 0 == strcmp("r8", s) ) {
        reg = REG_R8;
    } else if ( 0 == strcmp("r9l", s) ) {
        reg = REG_R9L;
    } else if ( 0 == strcmp("r9w", s) ) {
        reg = REG_R9W;
    } else if ( 0 == strcmp("r9d", s) ) {
        reg = REG_R9D;
    } else if ( 0 == strcmp("r9", s) ) {
        reg = REG_R9;
    } else if ( 0 == strcmp("r10l", s) ) {
        reg = REG_R10L;
    } else if ( 0 == strcmp("r10w", s) ) {
        reg = REG_R10W;
    } else if ( 0 == strcmp("r10d", s) ) {
        reg = REG_R10D;
    } else if ( 0 == strcmp("r10", s) ) {
        reg = REG_R10;
    } else if ( 0 == strcmp("r11l", s) ) {
        reg = REG_R11L;
    } else if ( 0 == strcmp("r11w", s) ) {
        reg = REG_R11W;
    } else if ( 0 == strcmp("r11d", s) ) {
        reg = REG_R11D;
    } else if ( 0 == strcmp("r11", s) ) {
        reg = REG_R11;
    } else if ( 0 == strcmp("r12l", s) ) {
        reg = REG_R12L;
    } else if ( 0 == strcmp("r12w", s) ) {
        reg = REG_R12W;
    } else if ( 0 == strcmp("r12d", s) ) {
        reg = REG_R12D;
    } else if ( 0 == strcmp("r12", s) ) {
        reg = REG_R12;
    } else if ( 0 == strcmp("r13l", s) ) {
        reg = REG_R13L;
    } else if ( 0 == strcmp("r13w", s) ) {
        reg = REG_R13W;
    } else if ( 0 == strcmp("r13d", s) ) {
        reg = REG_R13D;
    } else if ( 0 == strcmp("r13", s) ) {
        reg = REG_R13;
    } else if ( 0 == strcmp("r14l", s) ) {
        reg = REG_R14L;
    } else if ( 0 == strcmp("r14w", s) ) {
        reg = REG_R14W;
    } else if ( 0 == strcmp("r14d", s) ) {
        reg = REG_R14D;
    } else if ( 0 == strcmp("r14", s) ) {
        reg = REG_R14;
    } else if ( 0 == strcmp("r15l", s) ) {
        reg = REG_R15L;
    } else if ( 0 == strcmp("r15w", s) ) {
        reg = REG_R15W;
    } else if ( 0 == strcmp("r15d", s) ) {
        reg = REG_R15D;
    } else if ( 0 == strcmp("r15", s) ) {
        reg = REG_R15;
    } else if ( 0 == strcmp("cs", s) ) {
        reg = REG_CS;
    } else if ( 0 == strcmp("ds", s) ) {
        reg = REG_DS;
    } else if ( 0 == strcmp("es", s) ) {
        reg = REG_ES;
    } else if ( 0 == strcmp("fs", s) ) {
        reg = REG_FS;
    } else if ( 0 == strcmp("gs", s) ) {
        reg = REG_GS;
    } else if ( 0 == strcmp("flags", s) ) {
        reg = REG_FLAGS;
    } else if ( 0 == strcmp("eflags", s) ) {
        reg = REG_EFLAGS;
    } else if ( 0 == strcmp("rflags", s) ) {
        reg = REG_RFLAGS;
    } else {
        reg = REG_UNKNOWN;
    }

    return reg;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _ARCH_X86_64_EVAL_H
#define _ARCH_X86_64_EVAL_H

#include "../../las.h"
#include "reg.h"
#include <stdint.h>

typedef enum _x86_64_val_type {
    X86_64_VAL_IMM,
    X86_64_VAL_REG,
    X86_64_VAL_ADDR,
} x86_64_val_type_t;

typedef int x86_64_addr_flag_t;
#define X86_64_ADDR_BASE        1
#define X86_64_ADDR_DISP        2
#define X86_64_ADDR_OFFSET      4
#define X86_64_ADDR_SCALE       8

/*
 * Address operand
 * [base register + displacement + offset register * scalar multiplier]
 */
typedef struct _x86_64_addr {
    /* Flags for each field */
    x86_64_addr_flag_t flags;
    /* Specified address size */
    size_t saddrsize;
    /* Base register */
    x86_64_reg_t base;
    /* Displacement */
    int64_t disp;
    /* Offset register */
    x86_64_reg_t offset;
    /* Scale multiplier */
    int scale;
} x86_64_addr_t;

/*
 * Operand value
 */
typedef struct _x86_64_val {
    x86_64_val_type_t type;
    /* Specified operand size */
    size_t sopsize;
    union {
        /* Immediate value */
        int64_t imm;
        /* Register */
        x86_64_reg_t reg;
        /* Address operand */
        x86_64_addr_t addr;
    } u;
} x86_64_val_t;

/*
 * Encoded operand
 */
typedef struct _x86_64_encoded_operand {
    int opreg;
    struct {
        int r;
        int x;
        int b;
    } rex;
    int modrm;
    int sib;
    struct {
        size_t sz;
        int64_t val;
    } disp;
    struct {
        size_t sz;
        int64_t val;
    } imm;
} x86_64_enop_t;

/*
 * Encoded instruction
 */
typedef struct _x86_64_encoded_instruction {
    int prefix1;
    int prefix2;
    int prefix3;
    int prefix4;
    int rex;
    int opcode1;
    int opcode2;
    int opcode3;
    int modrm;
    int sib;
    struct {
        size_t sz;
        int64_t val;
    } disp;
    struct {
        size_t sz;
        int64_t val;
    } imm;
} x86_64_instr_t;


/*
 * Target
 */
typedef enum _x86_64_target {
    X86_64_O16,
    X86_64_O32,
    X86_64_O64,
} x86_64_target_t;

/*
 * Assembler option
 */
typedef struct _x86_64_asm_opt {
    x86_64_target_t tgt;
    int prefix;
    int suffix;
} x86_64_asm_opt_t;

#ifdef __cplusplus
extern "C" {
#endif

    x86_64_val_t * x86_64_eval_expr(expr_t *);
    x86_64_val_t * x86_64_eval_operand(operand_t *);

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_X86_64_EVAL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

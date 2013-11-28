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
    X86_64_VAL_MODRM,
} x86_64_val_type_t;

typedef int x86_64_modrm_flag_t;
#define X86_64_MODRM_BASE       1
#define X86_64_MODRM_DISP       2
#define X86_64_MODRM_OFFSET     4
#define X86_64_MODRM_SCALE      8

/*
 * ModRM
 * [base register + displacement + offset register * scalar multiplier]
 */
typedef struct _x86_64_modrm {
    x86_64_modrm_flag_t flags;
    /* Base register */
    x86_64_reg_t base;
    /* Displacement */
    int64_t disp;
    /* Offset register */
    x86_64_reg_t offset;
    /* Scale multiplier */
    int scale;
} x86_64_modrm_t;
typedef struct _x86_64_addr {
    x86_64_modrm_flag_t flags;
    /* Base register */
    x86_64_reg_t base;
    /* Displacement */
    int64_t disp;
    /* Offset register */
    x86_64_reg_t offset;
    /* Scale multiplier */
    int scale;
} x86_64_addr_t;

typedef struct _x86_64_val {
    x86_64_val_type_t type;
    /* Operand size */
    int opsize;
    union {
        /* Immediate value */
        uint64_t imm;
        /* Register */
        x86_64_reg_t reg;
        /* Address */
        x86_64_modrm_t modrm;
    } u;
} x86_64_val_t;

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

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
#include "main.h"
#include <stdint.h>

#define X86_64_VAR_MIN  -2147483648
#define X86_64_VAR_MAX  2147483647


typedef enum _x86_64_val_type {
    X86_64_VAL_IMM,
    X86_64_VAL_REG,
    X86_64_VAL_ADDR,
} x86_64_val_type_t;

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

#ifdef __cplusplus
extern "C" {
#endif

    x86_64_rval_t *
    x86_64_expr_fix(const x86_64_label_table_t *, const expr_t *);

    int
    x86_64_expr_range(const x86_64_label_table_t *, const expr_t *, int64_t *,
                      int64_t *);

    x86_64_opr_t *
    x86_64_estimate_operand(operand_t *);

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

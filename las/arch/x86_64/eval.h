/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _ARCH_X86_64_EVAL_H
#define _ARCH_X86_64_EVAL_H

#include "../../las.h"
#include <stdint.h>

typedef enum _val_type {
    VAL_IMM,
} val_type_t;

typedef struct _val {
    val_type_t type;
    union {
        uint64_t imm;
    } u;
} val_t;

#ifdef __cplusplus
extern "C" {
#endif

    val_t * x86_64_eval_expr(expr_t *);

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

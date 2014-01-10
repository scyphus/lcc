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

#ifdef __cplusplus
extern "C" {
#endif

    x86_64_rval_t *
    x86_64_expr_fix(const x86_64_label_table_t *, const expr_t *);

    int
    x86_64_expr_range(const x86_64_label_table_t *, const expr_t *, int64_t *,
                      int64_t *);


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

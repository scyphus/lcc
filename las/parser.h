/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _LAS_PARSER_H
#define _LAS_PARSER_H

#include "expr.h"
#include "las.h"

#ifdef __cplusplus
extern "C" {
#endif

    expr_t * parse_expr(pcode_t *);
    stmt_t * parse_instr(pcode_t *, token_t *);
    stmt_vector_t * parse(pcode_t *);

#ifdef __cplusplus
}
#endif

#endif /* _LAS_PARSER_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

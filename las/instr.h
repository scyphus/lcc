/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _LAS_INSTR_H
#define _LAS_INSTR_H

#include "las.h"

#ifdef __cplusplus
extern "C" {
#endif

    void operand_free(operand_t *);
    void operands_delete(operand_vector_t *);

    void opcode_vector_delete(opcode_vector_t *);

    instr_t * instr_new(opcode_vector_t *, operand_vector_t *);
    void instr_delete(instr_t *);
    void stmt_free(stmt_t *);

#ifdef __cplusplus
}
#endif

#endif /* _LAS_INSTR_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

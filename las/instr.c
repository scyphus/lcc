/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "las.h"
#include <stdlib.h>

/*
 * Free a prefixed expression
 */
void
pexpr_free(pexpr_t *pexpr)
{
    expr_free(pexpr->expr);
    free(pexpr);
}

/*
 * Free an operand
 */
void
operand_free(operand_t *op)
{
    switch ( op->type ) {
    case OPERAND_ADDR_EXPR:
    case OPERAND_EXPR:
        pexpr_free(op->op.pexpr);
        break;
    default:
        ;
    }
}

/*
 * Delete the operands
 */
void
operands_delete(operand_vector_t *vec)
{
    size_t i;

    for ( i = 0; i < mvector_size(vec); i++ ) {
        operand_free(mvector_at(vec, i));
    }
    mvector_delete(vec);
}

/*
 * Delete the opcode vector
 */
void
opcode_vector_delete(opcode_vector_t *vec)
{
    size_t i;

    for ( i = 0; i < mvector_size(vec); i++ ) {
        free(mvector_at(vec, i));
    }
    mvector_delete(vec);
}

/*
 * Create a new instruction
 */
instr_t *
instr_new(opcode_vector_t *opcode, operand_vector_t *operands)
{
    instr_t *instr;

    instr = malloc(sizeof(instr_t));
    if ( NULL == instr ) {
        return NULL;
    }
    instr->opcode = opcode;
    instr->operands = operands;

    return instr;
}

/*
 * Delete the instruction
 */
void
instr_delete(instr_t *instr)
{
    opcode_vector_delete(instr->opcode);
    operands_delete(instr->operands);
    free(instr);
}

/*
 * Free the statement
 */
void
stmt_free(stmt_t *stmt)
{
    switch ( stmt->type ) {
    case STMT_INSTR:
        instr_delete(stmt->u.instr);
        break;
    case STMT_LABEL:
        free(stmt->u.label);
        break;
    case STMT_GLOBAL:
        free(stmt->u.global);
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

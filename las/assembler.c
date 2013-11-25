/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "las.h"
#include "vector.h"
#include "expr.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if 0


/*
 * Convert expression to operand address
 */
int
_expr_to_addr(op_addr_t *addr, expr_t *expr)
{
    return -1;
}

/*
 * Convert expression to operand address
 */
int
_expr_to_regsym(op_regsym_t *addr, expr_t *expr)
{
    return -1;
}

int
_expr_to_expr()
{
    return 0;
}



/*
 * Extend the vector maximum size by the specified value if it's full
 */
struct vector *
vector_extend_if_full(struct vector *vec, size_t esz)
{
    size_t sz;
    size_t maxsz;
    int err;

    /* Obtain the current and maximum size */
    sz = vector_size(vec);
    maxsz = vector_max_size(vec);
    if ( sz >= maxsz ) {
        /* Full */
        sz = check_size_t_add(maxsz, esz, &err);
        if ( CHECKINT_NOERR != err ) {
            /* Error */
            return NULL;
        }
        return vector_reserve(vec, sz);
    } else {
        /* Vacant */
        return vec;
    }
}



/*
 * Parse one instruction
 */
void
parse_instr(pcode_t *pcode)
{
    token_t *tok;
    char *sym;

    tok = token_queue_cur(pcode->token_queue);
    /* Skip EOLs */
    while ( NULL != tok && TOK_EOL == tok->type ) {
        tok = token_queue_next(pcode->token_queue);
    }

    if ( NULL == tok ) {
        return;
    }

    switch ( tok->type ) {
    case TOK_INVAL:
        printf("Invalid token at line %zu and column %zu.\n", tok->pos.l,
               tok->pos.c);
        break;
    case TOK_KW_GLOBAL:
        tok = token_queue_next(pcode->token_queue);
        if ( TOK_SYMBOL != tok->type ) {
            /* Error */
        }
        printf("global %s\n", tok->val.sym);
        (void)token_queue_next(pcode->token_queue);
        break;
    case TOK_SYMBOL:
        sym = strdup(tok->val.sym);
        if ( NULL == sym ) {
            /* Can't allocate memory */
            return;
        }
        tok = token_queue_next(pcode->token_queue);
        if ( TOK_COLON == tok->type ) {
            /* Label token */
            printf("Label: %s\n", sym);
            /* Skip this colon */
            (void)token_queue_next(pcode->token_queue);
        } else {
            printf("Opcode: %s\n", sym);
            parse_operand(pcode);
        }
        break;
    default:
        /* Other: Error */
        ;
    }

}
#endif

/*
 * Assemble the input code
 */
void
assemble(pcode_t *pcode)
{
    assembler_t a;

    ssize_t (*hashfunc)(const void *) = NULL;
    int (*compare)(const void *, const void *) = NULL;

    a.symbols = hashtable_new(32, hashfunc, compare);

    parse(pcode);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

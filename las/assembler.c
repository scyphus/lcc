/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "las.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
parse_one_line(pcode_t *pcode)
{
    token_t *tok;

    tok = token_queue_cur(pcode->token_queue);
    /* Skip EOLs */
    while ( NULL != tok && TOK_EOL == tok->type ) {
        tok = token_queue_next(pcode->token_queue);
    }

    if ( NULL == tok ) {
        return;
    }

    /* Opcode must be symbol */
    if ( TOK_SYMBOL != tok->type ) {
        /* Parse error */
        return;
    }
    fwrite(tok->val.sym, 1, strlen(tok->val.sym), stdout);
    printf("\n");
    tok = token_queue_next(pcode->token_queue);
    if ( TOK_COLON == tok->type ) {
        /* Label token */
    } else {
        /* Read until the end of line */
        while ( NULL != tok && TOK_EOL != tok->type ) {
            tok = token_queue_next(pcode->token_queue);
        }
    }
}


void
assemble(pcode_t *pcode)
{
    token_t *tok;

    token_queue_rewind(pcode->token_queue);
    while ( tok = token_queue_cur(pcode->token_queue) ) {
        parse_one_line(pcode);
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

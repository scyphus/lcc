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

/*
 * Print the usage
 */
void
usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <input file>\n", prog);
    exit(EXIT_FAILURE);
}

/*
 * Main routine
 */
int
main(int argc, const char *const argv[])
{
    pcode_t *pcode;
    const char *fname;
    FILE *fp;
    string_t *str;

    if ( argc < 2 ) {
        usage(argv[0]);
    }
    fname = argv[1];

    /* Preprocess the code */
    pcode = preprocess(fname);
    if ( NULL == pcode ) {
        fprintf(stderr, "Fatal error\n");
    }

    while ( pcode->token_queue ) {
        break;
    }

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

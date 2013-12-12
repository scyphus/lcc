/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "las.h"
#include "parser.h"
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
    stmt_vector_t *vec;
    const char *fname;

    /* Get the input file */
    if ( argc < 2 ) {
        usage(argv[0]);
    }
    fname = argv[1];

    arch_x86_64_assemble(fname);

#if 0
    /* Preprocess the code */
    pcode = preprocess(fname);
    if ( NULL == pcode ) {
        fprintf(stderr, "Fatal error\n");
        return EXIT_FAILURE;
    }

    /* Parse the code */
    vec = parse(pcode);
    if ( NULL == vec ) {
        fprintf(stderr, "Parse error\n");
        return EXIT_FAILURE;
    }

    arch_assemble_x86_64(vec);
#endif

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

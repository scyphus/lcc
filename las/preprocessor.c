/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "las.h"
#include "mvector.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Return a new preprocessor instance
 */
preprocessor_t *
pp_new(void)
{
    preprocessor_t *pp;

    pp = malloc(sizeof(preprocessor_t));
    if ( NULL == pp ) {
        /* Memory error */
        return NULL;
    }
    pp->fix = mvector_new();

    return pp;
}

/*
 * Free a preprocessor instance
 */
void
pp_delete(preprocessor_t *pp)
{
    int i;
    token_fix_t *fix;

    for ( i = 0; i < mvector_size(pp->fix); i++ ) {
        fix = mvector_at(pp->fix, i);
        free(fix->kw);
        free(fix);
    }
    mvector_delete(pp->fix);
    free(pp);
}

/*
 * Preprocess
 */
pcode_t *
preprocess(preprocessor_t *pp, const char *fname)
{
    scode_t *scode;
    tcode_t *tcode;
    pcode_t *pcode;

    /* Read source code from the input file */
    scode = scode_read(fname);
    if ( NULL == scode ) {
        return NULL;
    }

    tcode = tokenize(pp, scode);
    if ( NULL == tcode ) {
        /* FIXME: Free scode */
        return NULL;
    }

    pcode = malloc(sizeof(pcode_t));
    if ( NULL == pcode ) {
        /* FIXME: Free scode and tcode */
        return NULL;
    }
    pcode->token_queue = tcode->token_queue;
    pcode->tree = NULL;

    return pcode;
}

/*
 * Register fix
 */
int
pp_register_fix(preprocessor_t *pp, const char *s)
{
    token_fix_t *fix;

    fix = malloc(sizeof(token_fix_t));
    if ( NULL == fix ) {
        return -1;
    }
    fix->kw = strdup(s);
    if ( NULL == fix->kw ) {
        free(fix);
        return -1;
    }

    if ( NULL == mvector_push_back(pp->fix, fix) ) {
        free(fix->kw);
        free(fix);
        return -1;
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

/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#ifndef _CHECKINT_H
#define _CHECKINT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define CHECKINT_NOERR          0
#define CHECKINT_EOVERFLOW      1
#define CHECKINT_EZERODIV       2

#ifdef __cplusplus
extern "C" {
#endif

    size_t check_size_t_add(size_t, size_t, int *);
    size_t check_size_t_sub(size_t, size_t, int *);
    size_t check_size_t_mul(size_t, size_t, int *);
    size_t check_size_t_div(size_t, size_t, int *);

#if 0
    ssize_t check_ssize_t_add(ssize_t, ssize_t, int *);
    ssize_t check_ssize_t_sub(ssize_t, ssize_t, int *);
    ssize_t check_ssize_t_mul(ssize_t, ssize_t, int *);
    ssize_t check_ssize_t_div(ssize_t, ssize_t, int *);
#endif

    int32_t check_int32_add(int32_t, int32_t, int *);
    int32_t check_int32_sub(int32_t, int32_t, int *);
    int32_t check_int32_mul(int64_t, int64_t, int *);
    int32_t check_int32_div(int32_t, int32_t, int *);

    uint32_t check_uint32_add(uint64_t, uint64_t, int *);
    uint32_t check_uint32_sub(uint32_t, uint32_t, int *);
    uint32_t check_uint32_mul(int64_t, int64_t, int *);
    uint32_t check_uint32_div(uint32_t, uint32_t, int *);

    uint64_t check_uint64_add(uint64_t, uint64_t, int *);
    uint64_t check_uint64_sub(uint64_t, uint64_t, int *);
    uint64_t check_uint64_mul(uint64_t, uint64_t, int *);
    uint64_t check_uint64_div(uint64_t, uint64_t, int *);

#ifdef __cplusplus
}
#endif

#endif /* _CHECKINT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "checkint.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>

size_t
check_size_t_add(size_t x, size_t y, int *err)
{
    if ( x > SIZE_T_MAX - y ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return x + y;
}

size_t
check_size_t_sub(size_t x, size_t y, int *err)
{
    if ( x < y ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return x - y;
}

size_t
check_size_t_mul(size_t x, size_t y, int *err)
{
    int i, j;
    size_t z;

    z = x * y;
    i = j = 0;
    /* Zero bit */
    if ( x == 0 || y == 0 ) {
        *err = CHECKINT_NOERR;
        return z;
    }
    do {
        i++;
    } while ( (x = (x>>1)) > 0 );
    do {
        j++;
    } while ( (y = (y>>1)) > 0 );
    if ( SIZE_T_MAX>>(i+j) ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return z;
}

size_t
check_size_t_div(size_t x, size_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }

    return x / y;
}

#if 0
ssize_t
check_ssize_t_add(ssize_t x, ssize_t y, int *err)
{
    *err = CHECKINT_NOERR;
    if ( y >= 0 ) {
        if ( x > SSIZE_MAX - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < SSIZE_MIN - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x + y;
}

ssize_t
check_ssize_t_sub(ssize_t x, ssize_t y, int *err)
{
    if ( y < 0 ) {
        if ( x > SSIZE_MAX + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < SSIZE_MIN + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x - y;
}

ssize_t
check_ssize_t_mul(ssize_t x, ssize_t y, int *err)
{
    int i, j;
    ssize_t z;

    z = x * y;
    i = j = 0;
    /* Zero bit */
    if ( x == 0 || y == 0 ) {
        return z;
    }
    if ( z/x == y && z/y == x ) {
        return z;
    }
    *err = CHECKINT_EOVERFLOW;

    return z;
}

ssize_t_t
check_ssize_t_div(ssize_t_t x, ssize_t_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }
    if ( y == -1 && y == SSIZE_MIN ) {
        *err = CHECKINT_EOVERFLOW;
        return 0;
    }

    return x / y;
}
#endif

uint32_t
check_uint32_add(uint64_t x, uint64_t y, int *err)
{
    uint64_t z;

    z = x + y;
    if ( z > UINT32_MAX ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return (uint32_t) z;
}

uint32_t
check_uint32_sub(uint32_t x, uint32_t y, int *err)
{
    if ( x < y ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return x - y;
}

uint32_t
check_uint32_mul(int64_t x, int64_t y, int *err)
{
    int64_t z;

    z = x * y;
    if ( z > UINT32_MAX || z < 0 ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return (uint32_t) z;
}

uint32_t
check_uint32_div(uint32_t x, uint32_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }

    return x / y;
}

int32_t
check_int32_add(int32_t x, int32_t y, int *err)
{
    *err = CHECKINT_NOERR;
    if ( y >= 0 ) {
        if ( x > INT32_MAX - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < INT32_MIN - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x + y;
}

int32_t
check_int32_sub(int32_t x, int32_t y, int *err)
{
    *err = CHECKINT_NOERR;
    if ( y < 0 ) {
        if ( x > INT32_MAX + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < INT32_MIN + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x - y;
}

int32_t
check_int32_mul(int64_t x, int64_t y, int *err)
{
    int64_t z;

    z = x * y;
    if ( z > INT32_MAX || z < INT32_MIN ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return (int32_t) z;
}

int32_t
check_int32_div(int32_t x, int32_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }
    if ( y == -1 && y == INT32_MIN ) {
        *err = CHECKINT_EOVERFLOW;
        return 0;
    }

    return x / y;
}

uint64_t
check_uint64_add(uint64_t x, uint64_t y, int *err)
{
    if ( x > UINT64_MAX - y ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return x + y;
}

uint64_t
check_uint64_sub(uint64_t x, uint64_t y, int *err)
{
    if ( x < y ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return x - y;
}

uint64_t
check_uint64_mul(uint64_t x, uint64_t y, int *err)
{
    int i, j;
    uint64_t z;

    z = x * y;
    i = j = 0;
    /* Zero bit */
    if ( x == 0 || y == 0 ) {
        *err = CHECKINT_NOERR;
        return z;
    }
    do {
        i++;
    } while ( (x = (x>>1)) > 0 );
    do {
        j++;
    } while ( (y = (y>>1)) > 0 );
    if ( UINT64_MAX>>(i+j) ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return z;
}

uint64_t
check_uint64_div(uint64_t x, uint64_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }

    return x / y;
}


int64_t
check_int64_add(int64_t x, int64_t y, int *err)
{
    *err = CHECKINT_NOERR;
    if ( y >= 0 ) {
        if ( x > INT64_MAX - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < INT64_MIN - y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x + y;
}

int64_t
check_int64_sub(int64_t x, int64_t y, int *err)
{
    *err = CHECKINT_NOERR;
    if ( y < 0 ) {
        if ( x > INT64_MAX + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    } else {
        if ( x < INT64_MIN + y ) {
            *err = CHECKINT_EOVERFLOW;
        }
    }

    return x - y;
}

int64_t
check_int64_mul(int64_t x, int64_t y, int *err)
{
    int64_t z;

    z = x * y;
    if ( z > INT64_MAX || z < INT64_MIN ) {
        *err = CHECKINT_EOVERFLOW;
    } else {
        *err = CHECKINT_NOERR;
    }

    return (int64_t) z;
}

int64_t
check_int64_div(int64_t x, int64_t y, int *err)
{
    if ( y == 0 ) {
        *err = CHECKINT_EZERODIV;
        return 0;
    }
    if ( y == -1 && y == INT64_MIN ) {
        *err = CHECKINT_EOVERFLOW;
        return 0;
    }

    return x / y;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

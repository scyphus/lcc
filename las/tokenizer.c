/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "las.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SWAPZ(a, b) (((a) != (b)) && ((a) ^= (b), (b) = (a) ^ (b), (a) ^= (b)))
#define EOS     -1
#define EFATAL  1
#define IS_DECINT_CHAR(c) (isdigit(c))
#define IS_HEXINT_CHAR(c) (isxdigit(c))

static __inline__ int
IS_SYMBOL_CHAR(int c)
{
    return (isalnum(c) || '_' == c || '.' == c) ? 1 : 0;
}
static __inline__ int
IS_BININT_CHAR(int c)
{
    return (c == '0' || c == '1') ? 1 : 0;
}
static __inline__ int
IS_OCTINT_CHAR(int c)
{
    return (c >= '0' && c <= '7') ? 1 : 0;
}
static __inline__ int
IS_NONZERODIGIT(int c)
{
    return (c != '0' && isdigit(c)) ? 1 : 0;
}


/*
 * Return the current character
 */
static int
_cur(tokenizer_t *tokenizer)
{
    if ( tokenizer->cur >= tokenizer->scode->str.len ) {
        /* End-of-string */
        return EOS;
    }
    return tokenizer->scode->str.s[tokenizer->cur];
}
static unsigned char *
_curp(tokenizer_t *tokenizer)
{
    if ( tokenizer->cur >= tokenizer->scode->str.len ) {
        /* End-of-string */
        return NULL;
    }
    return &tokenizer->scode->str.s[tokenizer->cur];
}

/*
 * Proceed the pointer and return the next character
 */
static int
_next(tokenizer_t *tokenizer)
{
    if ( tokenizer->cur >= tokenizer->scode->str.len ) {
        /* End-of-string */
        return EOS;
    }
    if ( '\n' == _cur(tokenizer) ) {
         tokenizer->pos.l++;
         tokenizer->pos.c = 0;
    } else {
        tokenizer->pos.c++;
    }

    tokenizer->cur++;
    return _cur(tokenizer);
}

/*
 * Skip white spaces
 */
static int
_skip_whitespaces(tokenizer_t *tokenizer)
{
    int c;
    int cnt;

    /* Reset the counter */
    cnt = 0;

    /* Process until the end of white-spaces */
    c = _cur(tokenizer);
    while ( c >= 0 ) {
        /* Skip ``\t'', ``\n'', ``\v'', ``\f'', ``\r'' and `` ''  */
        if ( !isspace(c) || '\n' == c ) {
            /* Not a white space */
            break;
        }
        c = _next(tokenizer);
    }

    /* Return the number of skipped chars */
    return cnt;
}

/*
 * Skip line comment
 */
static int
_skip_line_comment(tokenizer_t *tokenizer)
{
    int c;
    int cnt;

    /* Reset the counter */
    cnt = 0;

    /* Process until the end of white-spaces */
    c = _cur(tokenizer);
    while ( c >= 0 ) {
        if ( '\n' == c ) {
            /* LF */
            break;
        }
        c = _next(tokenizer);
    }

    /* Return the number of skipped chars */
    return cnt;
}

/*
 * Read the content of a file from FILE pointer
 */
scode_t *
scode_read(const char *fname)
{
    scode_t *scode;
    FILE *fp;
    char buf[4096];
    off_t len;
    off_t pos;
    size_t nr;

    /* Open the file */
    fp = fopen(fname, "r");

    if ( 0 != fseeko(fp, 0, SEEK_END) ) {
        goto error;
    }
    len = ftello(fp);
    if ( len < 0 ) {
        goto error;
    }
    if ( 0 != fseeko(fp, 0, SEEK_SET) ) {
        goto error;
    }

    /* Allocate memory for the content of the input file */
    scode = malloc(sizeof(scode_t));
    if ( NULL == scode ) {
        goto error;
    }
    scode->str.s = malloc(len);
    if ( NULL == scode->str.s ) {
        goto error1;
    }
    scode->str.len = len;

    pos = 0;
    for ( ;; ) {
        nr = fread(buf, 1, sizeof(buf), fp);
        if ( nr <= 0 ) {
            break;
        }
        (void)memcpy(scode->str.s + pos, buf, nr);
        pos += nr;
    }

    /* Copy the file name */
    scode->fname = strdup(fname);
    if ( NULL == scode->fname ) {
        goto error2;
    }

    goto done;

error2:
    free(scode->str.s);
error1:
    free(scode);
error:
    /* Close the file */
    (void)fclose(fp);
    scode = NULL;

done:
    return scode;
}

/*
 * Free source code
 */
void
scode_delete(scode_t *scode)
{
    free(scode->str.s);
    free(scode);
}


/*
 * Create a new token queue
 */
token_queue_t *
token_queue_new(void)
{
    token_queue_t *tq;

    tq = malloc(sizeof(token_queue_t));
    if ( NULL == tq ) {
        return NULL;
    }
    tq->head = NULL;
    tq->tail = NULL;
    tq->cur = NULL;

    return tq;
}

/*
 * Insert a token to the tail of a token queue
 */
int
token_queue_insert_tail(token_queue_t *tq, token_t *token)
{
    /* new entry */
    token_queue_entry_t *ent;

    /* allocate the entry */
    ent = malloc(sizeof(token_queue_entry_t));
    if ( NULL == ent ) {
        return -1;
    }

    /* set token */
    ent->token = token;
    ent->next = NULL;

    /* insert into tail */
    if ( NULL == tq->head ) {
        tq->head = ent;
        tq->tail = ent;
        tq->cur = ent;
    } else {
        tq->tail->next = ent;
        tq->tail = ent;
    }

    return 0;
}

/*
 * Rewind the token queue
 */
void
token_queue_rewind(token_queue_t *tq)
{
    tq->cur = tq->head;
}

/*
 * Get the current token from the queue
 */
token_t *
token_queue_cur(token_queue_t *tq)
{
    if ( NULL == tq->cur ) {
        /* End-of-token */
        return NULL;
    }

    return tq->cur->token;
}

/*
 * Proceed the queue to the next token and return it
 */
token_t *
token_queue_next(token_queue_t *tq)
{
    if ( NULL == tq->cur ) {
        return NULL;
    }
    tq->cur = tq->cur->next;

    return token_queue_cur(tq);
}

/*
 * Allocate new token
 */
token_t *
token_new(tokenizer_t * tokenizer)
{
    token_t *token;

    token = malloc(sizeof(token_t));
    if ( NULL == token ) {
        return NULL;
    }
    token->scode = tokenizer->scode;
    token->pos.l = tokenizer->pos.l;
    token->pos.c = tokenizer->pos.c;
    token->off = tokenizer->cur;

    return token;
}

/*
 * Append a typed token
 */
int
_append_typed_token(tokenizer_t *tokenizer, token_queue_t *tq,
                    token_type_t type)
{
    token_t *token;

    token = token_new(tokenizer);
    if ( NULL == token ) {
        return -1;
    }
    token->type = type;

    return token_queue_insert_tail(tq, token);
}

/*
 * Scan number
 *
 * binint ::= 0 b bindigit+
 * octint ::= 0 octdigit+
 * decint ::= nonzerodigit digit*
 * hexint ::= 0 x hexdigit+
 *
 * bindigit ::= "0" | "1"
 * octdigit ::= "0"..."7"
 * hexdigit ::= "0"..."9" | "a"..."f" | "A"..."F"
 * nonzerodigit ::= "1"..."9"
 */
static int
_scan_number(tokenizer_t *tokenizer, token_queue_t *tq)
{
    int c;
    int c0;
    unsigned char *sp;
    unsigned char *tp;
    char *val;
    int ret;
    int type;
    token_t *token;

    /* Store the current pointer */
    sp = _curp(tokenizer);

    /* Get the current and the next character */
    c0 = _cur(tokenizer);
    c = _next(tokenizer);

    if ( '0' == c0 && IS_OCTINT_CHAR(c) ) {
        /* Octet */
        type = TOK_OCTINT;
        while ( IS_OCTINT_CHAR(c) ) {
            c = _next(tokenizer);
        }
    } else if ( '0' == c0 && 'b' == c ) {
        /* Binary */
        type = TOK_BININT;
        c = _next(tokenizer);
        if ( !IS_BININT_CHAR(c) ) {
            /* Error: Invalid token */
            type = TOK_INVAL;
        } else {
            while ( IS_BININT_CHAR(c) ) {
                c = _next(tokenizer);
            }
        }
    } else if ( '0' == c0 && 'x' == c ) {
        /* Hex */
        type = TOK_HEXINT;
        c = _next(tokenizer);
        if ( !IS_HEXINT_CHAR(c) ) {
            /* Error: Invalid token */
            type = TOK_INVAL;
        } else {
            while ( IS_HEXINT_CHAR(c) ) {
                c = _next(tokenizer);
            }
        }
    } else {
        type = TOK_DECINT;
        /* Decimal */
        while ( IS_DECINT_CHAR(c) ) {
            c = _next(tokenizer);
        }
    }

    /* Get the end of the number */
    tp = _curp(tokenizer);

    /* Check the length of the string */
    if ( sp == tp ) {
        /* Note: Must not reach here */
        (void)_next(tokenizer);
        type = TOK_INVAL;
    }

    if ( TOK_INVAL == type ) {
        /* Found it's an invalid token */
        ret = _append_typed_token(tokenizer, tq, TOK_INVAL);
    } else {
        /* Copy the value */
        val = malloc(sizeof(char) * (tp - sp + 1));
        if ( NULL == val ) {
            return -1;
        }
        (void)memcpy(val, sp, tp - sp);
        val[tp - sp] = '\0';

        /* Allocate a token */
        token = token_new(tokenizer);
        if ( NULL == token ) {
            free(val);
            return -1;
        }
        token->type = type;
        token->val.num = val;

        ret = token_queue_insert_tail(tq, token);
    }

    return ret;
}

static int
_strmemcmp(const unsigned char *a0, const unsigned char *a1, const char *b)
{
    size_t len;

    len = strlen(b);
    if ( a1 - a0 == len && 0 == memcmp(a0, b, len) ) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Scan symbol
 */
static int
_scan_symbol(preprocessor_t *pp, tokenizer_t *tokenizer, token_queue_t *tq)
{
    int i;
    int c;
    unsigned char *sp;
    unsigned char *tp;
    char *val;
    int ret;
    token_t *token;
    token_fix_t *fix;
    token_type_t type;

    /* Store the current pointer */
    sp = _curp(tokenizer);

    /* Get the curret character */
    c = _cur(tokenizer);
    while ( IS_SYMBOL_CHAR(c) ) {
        c = _next(tokenizer);
    }

    /* Get the end of the symbol */
    tp = _curp(tokenizer);

    /* Check the length of the string */
    if ( sp == tp ) {
        /* Invalidate if the length is zero */
        (void)_next(tokenizer);
        ret = _append_typed_token(tokenizer, tq, TOK_INVAL);
    } else if ( 0 == _strmemcmp(sp, tp, "global") ) {
        ret = _append_typed_token(tokenizer, tq, TOK_KW_GLOBAL);
    } else if ( 0 == _strmemcmp(sp, tp, "byte") ) {
        ret = _append_typed_token(tokenizer, tq, TOK_KW_BYTE);
    } else if ( 0 == _strmemcmp(sp, tp, "word") ) {
        ret = _append_typed_token(tokenizer, tq, TOK_KW_WORD);
    } else if ( 0 == _strmemcmp(sp, tp, "dword") ) {
        ret = _append_typed_token(tokenizer, tq, TOK_KW_DWORD);
    } else if ( 0 == _strmemcmp(sp, tp, "qword") ) {
        ret = _append_typed_token(tokenizer, tq, TOK_KW_QWORD);
    } else {
        type = TOK_SYMBOL;
        /* Search from the registered fix keywords */
        for ( i = 0; i < mvector_size(pp->fix); i++ ) {
            fix = mvector_at(pp->fix, i);
            if ( tp - sp == strlen(fix->kw)
                 && 0 == memcmp(sp, fix->kw, strlen(fix->kw)) ) {
                type = TOK_FIX;
                break;
            }
        }

        /* Copy the value */
        val = malloc(sizeof(char) * (tp - sp + 1));
        if ( NULL == val ) {
            return -1;
        }
        (void)memcpy(val, sp, tp - sp);
        val[tp - sp] = '\0';

        /* Allocate a token */
        token = token_new(tokenizer);
        if ( NULL == token ) {
            free(val);
            return -1;
        }
        token->type = type;
        token->val.sym = val;

        ret = token_queue_insert_tail(tq, token);
    }

    return ret;
}

/*
 * Proceed to the next token
 */
int
_next_token(preprocessor_t *pp, tokenizer_t *tokenizer, token_queue_t *tq)
{
    int c;
    int ret;

    /* Skip white spaces */
    _skip_whitespaces(tokenizer);

    /* Get the current character */
    c = _cur(tokenizer);

    /* Reset return code */
    ret = 0;

    switch ( c ) {
    case EOS:
        /* End-of-file */
        ret = -1;
        break;
    case '\n':
        ret = _append_typed_token(tokenizer, tq, TOK_EOL);
        (void)_next(tokenizer);
        break;
    case '#':
        ret = _append_typed_token(tokenizer, tq, TOK_SHARP);
        (void)_next(tokenizer);
        break;
    case ',':
        ret = _append_typed_token(tokenizer, tq, TOK_COMMA);
        (void)_next(tokenizer);
        break;
    case ':':
        ret = _append_typed_token(tokenizer, tq, TOK_COLON);
        (void)_next(tokenizer);
        break;
    case ';':
        ret = _append_typed_token(tokenizer, tq, TOK_SEMICOLON);
        (void)_next(tokenizer);
        break;
    case '[':
        ret = _append_typed_token(tokenizer, tq, TOK_LBRACKET);
        (void)_next(tokenizer);
        break;
    case ']':
        ret = _append_typed_token(tokenizer, tq, TOK_RBRACKET);
        (void)_next(tokenizer);
        break;
    case '(':
        ret = _append_typed_token(tokenizer, tq, TOK_LPAREN);
        (void)_next(tokenizer);
        break;
    case ')':
        ret = _append_typed_token(tokenizer, tq, TOK_RPAREN);
        (void)_next(tokenizer);
        break;
    case '+':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_PLUS);
        (void)_next(tokenizer);
        break;
    case '-':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_MINUS);
        (void)_next(tokenizer);
        break;
    case '*':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_MUL);
        (void)_next(tokenizer);
        break;
    case '/':
        c = _next(tokenizer);
        if ( '/' == c ) {
            /* Comment */
            _skip_line_comment(tokenizer);
        } else {
            ret = _append_typed_token(tokenizer, tq, TOK_OP_DIV);
        }
        break;
    case '~':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_TILDE);
        (void)_next(tokenizer);
        break;
    case '<':
        c = _next(tokenizer);
        if ( '<' == c ) {
            ret = _append_typed_token(tokenizer, tq, TOK_OP_LSHIFT);
            (void)_next(tokenizer);
        } else {
            /* Invalid */
            ret = _append_typed_token(tokenizer, tq, TOK_INVAL);
        }
        break;
    case '>':
        c = _next(tokenizer);
        if ( '>' == c ) {
            ret = _append_typed_token(tokenizer, tq, TOK_OP_RSHIFT);
            (void)_next(tokenizer);
        } else {
            /* Invalid */
            ret = _append_typed_token(tokenizer, tq, TOK_INVAL);
        }
        break;
    case '|':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_BAR);
        (void)_next(tokenizer);
        break;
    case '&':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_AMP);
        (void)_next(tokenizer);
        break;
    case '^':
        ret = _append_typed_token(tokenizer, tq, TOK_OP_XOR);
        (void)_next(tokenizer);
        break;
    default:
        /* Other characters */
        if ( isdigit(c) ) {
            /* Starting with digit; then scan number */
            ret = _scan_number(tokenizer, tq);
        } else {
            /* else; then scan symbol */
            ret = _scan_symbol(pp, tokenizer, tq);
        }
    }

    return ret;
}


/*
 * Tokenize
 */
tcode_t *
tokenize(preprocessor_t *pp, scode_t *scode)
{
    tcode_t *tcode;
    tokenizer_t tokenizer;
    int ret;

    /* Allocate tcode */
    tcode = malloc(sizeof(tcode_t));
    if ( NULL == tcode ) {
        return NULL;
    }
    tcode->scode = scode;
    tcode->token_queue = token_queue_new();

    tokenizer.scode = scode;
    tokenizer.cur = 0;
    tokenizer.pos.l = 0;
    tokenizer.pos.c = 0;

    for ( ;; ) {
        ret = _next_token(pp, &tokenizer, tcode->token_queue);
        if ( ret < 0 ) {
            break;
        }
    }
    /*printf("%lld [%zu:%zu]\n", tokenizer.cur, tokenizer.pos.l, tokenizer.pos.c);*/

    return tcode;
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

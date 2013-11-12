/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "vector.h"
#include "expr.h"
#include "las.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


/*
 * Create new infix operator
 */
expr_t *
expr_infix_operator(expr_operator_type_t type, expr_t *expr0, expr_t *expr1)
{
    expr_t *expr;
    expr_vec_t *vec;

    vec = vector_new(2);
    if ( NULL == vector_push_back(vec, expr0) ) {
        vector_delete(vec);
        return NULL;
    }
    if ( NULL == vector_push_back(vec, expr1) ) {
        vector_delete(vec);
        return NULL;
    }

    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        vector_delete(vec);
        return NULL;
    }
    expr->type = EXPR_OP;
    expr->u.op.type = type;
    expr->u.op.fix_type = FIX_INFIX;
    expr->u.op.args = vec;

    return expr;
}

/*
 * Create new prefix operator
 */
expr_t *
expr_prefix_operator(expr_operator_type_t type, expr_t *expr0)
{
    expr_t *expr;
    expr_vec_t *vec;

    vec = vector_new(1);
    if ( NULL == vector_push_back(vec, expr0) ) {
        vector_delete(vec);
        return NULL;
    }

    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        vector_delete(vec);
        return NULL;
    }
    expr->type = EXPR_OP;
    expr->u.op.type = type;
    expr->u.op.fix_type = FIX_PREFIX;
    expr->u.op.args = vec;

    return expr;
}

/*
 * Variable (symbol or label) expression
 */
expr_t *
expr_var(char *var)
{
    expr_t *expr;

    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        return NULL;
    }
    expr->type = EXPR_VAR;
    expr->u.var = strdup(var);
    if ( NULL == expr->u.var ) {
        free(expr);
        return NULL;
    }

    return expr;
}

/*
 * Integer expression
 */
expr_t *
expr_int(token_t *tok)
{
    expr_t *expr;
    unsigned long long val;

    switch ( tok->type ) {
    case TOK_BININT:
        val = strtoull(tok->val.num + 2, NULL, 2);
        break;
    case TOK_OCTINT:
        val = strtoull(tok->val.num + 1, NULL, 8);
        break;
    case TOK_DECINT:
        val = strtoull(tok->val.num, NULL, 10);
        break;
    case TOK_HEXINT:
        val = strtoull(tok->val.num, NULL, 16);
        break;
    default:
        return NULL;
    }

    expr = malloc(sizeof(expr_t));
    if ( NULL == expr ) {
        return NULL;
    }
    expr->type = EXPR_INT;
    expr->u.i = val;

    return expr;
}



/*
 * Free expression instance
 */
void
expr_free(expr_t *expr)
{
    /* FIXME */
    switch ( expr->type ) {
    default:
        ;
    }
    free(expr);
}


/*
 * Parse expression atom
 *
 * atom ::=
 *              symbol | literal | "(" expression ")"
 */
expr_t *
parse_expr_atom(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        return NULL;
    }
    switch ( tok->type ) {
    case TOK_SYMBOL:
        /* Parse symbol expression */
        expr = expr_var(tok->val.sym);
        /* Eat */
        (void)token_queue_next(pcode->token_queue);
        break;
    case TOK_BININT:
    case TOK_OCTINT:
    case TOK_DECINT:
    case TOK_HEXINT:
        /* Parse integer expression */
        expr = expr_int(tok);
        /* Eat */
        (void)token_queue_next(pcode->token_queue);
        break;
    case TOK_LPAREN:
        /* Eat TOK_LPAREN */
        (void)token_queue_next(pcode->token_queue);
        expr = parse_expr(pcode);
        if ( NULL == expr ) {
            /* Parse error */
            return NULL;
        }
        /* Must end with RPAREN */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok || TOK_RPAREN != tok->type ) {
            /* Syntax error */
            expr_free(expr);
            return NULL;
        }
        /* Eat */
        (void)token_queue_next(pcode->token_queue);
        break;
    default:
        /* Syntax error */
        return NULL;
    }

    return expr;
}

/*
 * Parse expression primary
 *
 * primary ::=
 *              atom
 */
expr_t *
parse_expr_primary(pcode_t *pcode)
{
    return parse_expr_atom(pcode);
}

/*
 * Parse expression u_expr
 *
 * u_expr ::=
 *              primary | "-" u_expr | "+" u_expr | "~" u_expr
 */
expr_t *
parse_expr_u_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        return NULL;
    }
    switch ( tok->type ) {
    case TOK_OP_MINUS:
        /* Eat TOK_OP_MINUS */
        (void)token_queue_next(pcode->token_queue);
        expr0 = parse_expr_u_expr(pcode);
        if ( NULL == expr0 ) {
            /* Parse error */
            return NULL;
        }
        expr = expr_prefix_operator(OP_MINUS, expr0);
        if ( NULL == expr ) {
            /* Error and free locally allocated memory */
            expr_free(expr0);
            return NULL;
        }
        break;
    case TOK_OP_PLUS:
        /* Eat TOK_OP_PLUS */
        (void)token_queue_next(pcode->token_queue);
        expr0 = parse_expr_u_expr(pcode);
        if ( NULL == expr0 ) {
            /* Parse error */
            return NULL;
        }
        expr = expr_prefix_operator(OP_PLUS, expr0);
        if ( NULL == expr ) {
            /* Error and free locally allocated memory */
            expr_free(expr0);
            return NULL;
        }
        break;
    case TOK_OP_TILDE:
        /* Eat TOK_OP_PLUS */
        (void)token_queue_next(pcode->token_queue);
        expr0 = parse_expr_u_expr(pcode);
        if ( NULL == expr0 ) {
            /* Parse error */
            return NULL;
        }
        expr = expr_prefix_operator(OP_TILDE, expr0);
        if ( NULL == expr ) {
            /* Error and free locally allocated memory */
            expr_free(expr0);
            return NULL;
        }
        break;
    default:
        /* Primary */
        expr = parse_expr_primary(pcode);
        if ( NULL == expr ) {
            /* Error */
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression m_expr
 *
 * m_expr ::=
 *              u_expr ( ("*" | "/") u_expr )*
 */
expr_t *
parse_expr_m_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first `u' expression */
    expr = parse_expr_u_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_MUL == tok->type || TOK_OP_DIV == tok->type ) {
        if ( TOK_OP_MUL == tok->type ) {
            /* Eat TOK_OP_MUL */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `u' expression */
            expr1 = parse_expr_u_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_MUL, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        } else if ( TOK_OP_DIV == tok->type ) {
            /* Eat TOK_OP_DIV */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `u' expression */
            expr1 = parse_expr_u_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_DIV, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression a_expr
 *
 * a_expr ::=
 *              m_expr ( ("+" | "-") m_expr )*
 */
expr_t *
parse_expr_a_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first `m' expression */
    expr = parse_expr_m_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_PLUS == tok->type || TOK_OP_MINUS == tok->type ) {
        if ( TOK_OP_PLUS == tok->type ) {
            /* Eat TOK_OP_PLUS */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `m' expression */
            expr1 = parse_expr_m_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_PLUS, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        } else if ( TOK_OP_MINUS == tok->type ) {
            /* Eat TOK_OP_MINUS */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `m' expression */
            expr1 = parse_expr_m_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_MINUS, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression shift_expr
 *
 * shift_expr ::=
 *              a_expr ( ("<<" | ">>") a_expr )*
 */
expr_t *
parse_expr_shift_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first `a' expression */
    expr = parse_expr_a_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_LSHIFT == tok->type || TOK_OP_RSHIFT == tok->type ) {
        if ( TOK_OP_LSHIFT == tok->type ) {
            /* Eat TOK_OP_LSHIFT */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `a' expression */
            expr1 = parse_expr_a_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_LSHIFT, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        } else if ( TOK_OP_RSHIFT == tok->type ) {
            /* Eat TOK_OP_RSHIFT */
            (void)token_queue_next(pcode->token_queue);
            expr0 = expr;
            /* Parse the second `a' expression */
            expr1 = parse_expr_a_expr(pcode);
            if ( NULL == expr1 ) {
                /* Error */
                expr_free(expr0);
                return NULL;
            }
            expr = expr_infix_operator(OP_RSHIFT, expr0, expr1);
            if ( NULL == expr ) {
                /* Error */
                expr_free(expr0);
                expr_free(expr1);
                return NULL;
            }
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression and_expr
 *
 * and_expr ::=
 *              shift_expr ( "&" shift_expr )*
 */
expr_t *
parse_expr_and_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first shift expression */
    expr = parse_expr_shift_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_AMP == tok->type ) {
        /* Eat TOK_OP_AMP */
        (void)token_queue_next(pcode->token_queue);
        expr0 = expr;
        /* Parse the second shift expression */
        expr1 = parse_expr_shift_expr(pcode);
        if ( NULL == expr1 ) {
            /* Error */
            expr_free(expr0);
            return NULL;
        }
        expr = expr_infix_operator(OP_AMP, expr0, expr1);
        if ( NULL == expr ) {
            /* Error */
            expr_free(expr0);
            expr_free(expr1);
            return NULL;
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression xor_expr
 *
 * xor_expr ::=
 *              and_expr ( "^" and_expr )*
 */
expr_t *
parse_expr_xor_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first and expression */
    expr = parse_expr_and_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_XOR == tok->type ) {
        /* Eat TOK_OP_XOR */
        (void)token_queue_next(pcode->token_queue);
        expr0 = expr;
        /* Parse the second and expression */
        expr1 = parse_expr_and_expr(pcode);
        if ( NULL == expr1 ) {
            /* Error */
            expr_free(expr0);
            return NULL;
        }
        expr = expr_infix_operator(OP_XOR, expr0, expr1);
        if ( NULL == expr ) {
            /* Error */
            expr_free(expr0);
            expr_free(expr1);
            return NULL;
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression or_expr
 *
 * or_expr ::=
 *              xor_expr ( "|" xor_expr )*
 */
expr_t *
parse_expr_or_expr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    expr_t *expr0;
    expr_t *expr1;

    /* Parse the first xor expression */
    expr = parse_expr_xor_expr(pcode);
    if ( NULL == expr ) {
        /* Error */
        return NULL;
    }

    /* Check the next token */
    tok = token_queue_cur(pcode->token_queue);
    if ( NULL == tok ) {
        /* End of token; i.e., syntax error */
        expr_free(expr);
        return NULL;
    }
    while ( TOK_OP_BAR == tok->type ) {
        /* Eat TOK_OP_BAR */
        (void)token_queue_next(pcode->token_queue);
        expr0 = expr;
        /* Parse the second xor expression */
        expr1 = parse_expr_xor_expr(pcode);
        if ( NULL == expr1 ) {
            /* Error */
            expr_free(expr0);
            return NULL;
        }
        expr = expr_infix_operator(OP_BAR, expr0, expr1);
        if ( NULL == expr ) {
            /* Error */
            expr_free(expr0);
            expr_free(expr1);
            return NULL;
        }

        /* Get the current token */
        tok = token_queue_cur(pcode->token_queue);
        if ( NULL == tok ) {
            /* Error */
            expr_free(expr);
            return NULL;
        }
    }

    return expr;
}

/*
 * Parse expression
 *
 * expression ::=
 *              or_expr
 */
expr_t *
parse_expr(pcode_t *pcode)
{
    return parse_expr_or_expr(pcode);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

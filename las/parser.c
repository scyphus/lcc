/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#include "expr.h"
#include "parser.h"
#include "instr.h"
#include "las.h"
#include "checkint.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
    unsigned long long ival;

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
        /* Convert a token to an integer value */
        ival = strtoull(tok->val.num + 2, NULL, 2);
    case TOK_OCTINT:
        /* Convert a token to an integer value */
        ival = strtoull(tok->val.num + 1, NULL, 8);
    case TOK_DECINT:
        /* Convert a token to an integer value */
        ival = strtoull(tok->val.num, NULL, 10);
    case TOK_HEXINT:
        /* Convert a token to an integer value */
        ival = strtoull(tok->val.num, NULL, 16);
        /* Create new integer expression */
        expr = expr_int(ival);
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
 * Parse an expr
 *
 * operand_expr ::=
 *              expression
 */
operand_t *
parse_operand_expr(pcode_t *pcode)
{
    expr_t *expr;
    operand_t *op;

    expr = parse_expr(pcode);
    if ( NULL == expr ) {
        /* Parse error */
        return NULL;
    }
    /* Expression to op_regsym_t */
    op = malloc(sizeof(operand_t));
    if ( NULL == op ) {
        expr_free(expr);
        return NULL;
    }
    op->type = OPERAND_EXPR;
    op->op.expr = expr;

    return op;
}

/*
 * Parse an addr or moffset
 *
 * operand_addr ::=
 *              "[" expression "]"
 */
operand_t *
parse_operand_addr(pcode_t *pcode)
{
    token_t *tok;
    expr_t *expr;
    operand_t *op;

    /* Skip lbracket */
    (void)token_queue_next(pcode->token_queue);

    /* Parse expression */
    expr = parse_expr(pcode);
    if ( NULL == expr ) {
        /* Parse error */
        return NULL;
    }

    /* Get the current token */
    tok = token_queue_cur(pcode->token_queue);
    if ( tok == NULL || TOK_RBRACKET != tok->type ) {
        return NULL;
    }
    /* Skip the TOK_RBRACKET */
    (void)token_queue_next(pcode->token_queue);

    /* Expression to addr */
    op = malloc(sizeof(operand_t));
    if ( NULL == op ) {
        expr_free(expr);
        return NULL;
    }
    op->type = OPERAND_ADDR_EXPR;
    op->op.expr = expr;

    return op;
}

/*
 * Parse an operand
 *
 * operand ::=
 *              ( operand_expr | operand_addr )
 */
operand_t *
parse_operand(pcode_t *pcode)
{
    operand_t *op;
    token_t *tok;

    /* Get the current token and check it */
    tok = token_queue_cur(pcode->token_queue);
    if ( TOK_BININT == tok->type || TOK_OCTINT == tok->type
         || TOK_DECINT == tok->type || TOK_HEXINT == tok->type
         || TOK_OP_PLUS == tok->type || TOK_OP_MINUS == tok->type
         || TOK_OP_TILDE == tok->type || TOK_SYMBOL == tok->type ) {
        /* Symbol, register, or immediate value */
        op = parse_operand_expr(pcode);
    } else if ( TOK_LBRACKET == tok->type ) {
        /* Address or moffset */
        op = parse_operand_addr(pcode);
    } else {
        /* Parse error */
        return NULL;
    }

    return op;
}

/*
 * Parse instruction
 *
 * instruction ::=
 *              opcode operand ( "," operand )*
 */
stmt_t *
parse_instr(pcode_t *pcode, const char *sym)
{
    stmt_t *stmt;
    instr_t *instr;
    token_t *tok;
    char *opcode;
    operand_t *op;
    operand_vector_t *vec;

    vec = mvector_new();

    /* Read until the end of line */
    tok = token_queue_cur(pcode->token_queue);
    while ( NULL != tok && TOK_EOL != tok->type ) {
        op = parse_operand(pcode);
        if ( NULL == op ) {
            /* Free operands */
            operands_delete(vec);
            return NULL;
        }
        if ( NULL == mvector_push_back(vec, op) ) {
            operand_free(op);
            /* Free operands */
            operands_delete(vec);
            return NULL;
        }
        tok = token_queue_cur(pcode->token_queue);
        if ( TOK_COMMA == tok->type ) {
            /* Proceed to the next operand */
            (void)token_queue_next(pcode->token_queue);
        } else if ( TOK_EOL == tok->type ) {
            /* EOL (Valid syntax) */
        } else {
            /* Syntax error */
            (void)token_queue_next(pcode->token_queue);
            /* Free operands */
            operands_delete(vec);
            return NULL;
        }
    }

    /* Opcode */
    opcode = strdup(sym);
    if ( NULL == opcode ) {
        operands_delete(vec);
        return NULL;
    }

    /* Instruction */
    instr = instr_new(opcode, vec);
    if ( NULL == instr ) {
        free(opcode);
        operands_delete(vec);
        return NULL;
    }

    /* Allocate for the global statement structure */
    stmt = malloc(sizeof(stmt_t));
    if ( NULL == stmt ) {
        /* Error on memory allocation */
        instr_delete(instr);
        return NULL;
    }
    /* Set attributes */
    stmt->type = STMT_INSTR;
    stmt->u.instr = instr;

    return stmt;
}

/*
 * Parse label
 *
 * label ::=
 *              symbol ":"
 */
stmt_t *
parse_label(pcode_t *pcode, const char *sym)
{
    stmt_t *stmt;

    /* Allocate for the global statement structure */
    stmt = malloc(sizeof(stmt_t));
    if ( NULL == stmt ) {
        /* Error on memory allocation */
        return NULL;
    }
    /* Set attributes */
    stmt->type = STMT_LABEL;
    stmt->u.label = strdup(sym);
    if ( NULL == stmt->u.label ) {
        /* Can't allocate memory */
        free(stmt);
        return NULL;
    }

    /* Skip this colon */
    (void)token_queue_next(pcode->token_queue);

    return stmt;
}

/*
 * Parse global statement
 *
 * global ::=
 *              "global" symbol
 */
stmt_t *
parse_global(pcode_t *pcode)
{
    token_t *tok;
    stmt_t *stmt;

    /* Skip "global" and get the symbol string */
    tok = token_queue_next(pcode->token_queue);
    if ( TOK_SYMBOL != tok->type ) {
        /* Syntax error */
        return NULL;
    }

    /* Allocate for the global statement structure */
    stmt = malloc(sizeof(stmt_t));
    if ( NULL == stmt ) {
        /* Error on memory allocation */
        return NULL;
    }
    /* Set attributes */
    stmt->type = STMT_GLOBAL;
    stmt->u.global = strdup(tok->val.sym);
    if ( NULL == stmt->u.global ) {
        /* Error on memory allocation */
        free(stmt);
        return NULL;
    }

    /* Skip the current token */
    (void)token_queue_next(pcode->token_queue);

    return stmt;
}

/*
 * Parse a sequence of tokens from the input pcode
 *
 * input ::=
 *              ( EOL | instruction | label | global )* EOF
 */
stmt_vector_t *
parse(pcode_t *pcode)
{
    token_t *tok;
    char *sym;
    stmt_t *stmt;
    stmt_vector_t *vec;

    /* Create a new statement vector */
    vec = mvector_new();

    token_queue_rewind(pcode->token_queue);
    while ( NULL != (tok = token_queue_cur(pcode->token_queue)) ) {
        switch ( tok-> type ) {
        case TOK_INVAL:
            /* Invalid token */
            /* FIXME: Raise an error */
            return NULL;
            break;
        case TOK_EOL:
            (void)token_queue_next(pcode->token_queue);
            break;
        case TOK_KW_GLOBAL:
            /* glboal */
            stmt = parse_global(pcode);
            if ( NULL == mvector_push_back(vec, stmt) ) {
                stmt_free(stmt);
                return NULL;
            }
            break;
        case TOK_SYMBOL:
            /* Store the symbol */
            sym = strdup(tok->val.sym);
            if ( NULL == sym ) {
                return NULL;
            }
            tok = token_queue_next(pcode->token_queue);
            if ( TOK_COLON == tok->type ) {
                /* Label token */
                stmt = parse_label(pcode, sym);
            } else {
                stmt = parse_instr(pcode, sym);
            }
            free(sym);
            if ( NULL == stmt ) {
                /* Parse error */
                return NULL;
            }
            if ( NULL == mvector_push_back(vec, stmt) ) {
                stmt_free(stmt);
                return NULL;
            }
            break;
        default:
            /* Other token */
            return NULL;
        }
    }

    return vec;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

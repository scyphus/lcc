/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

#ifndef _LAS_H
#define _LAS_H

#include "token.h"
#include "vector.h"
#include "mvector.h"
#include "expr.h"
#include "hashtable.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define SIZE8   1
#define SIZE16  2
#define SIZE32  4
#define SIZE64  8

/* Errors */
#define EGENERIC        1
#define EUNKNOWN        2
#define EOPERAND        3
#define ESIZE           4
#define EPREFIX         5
#define EDUP            6

/*
 * String (can be binary)
 */
typedef struct _string {
    unsigned char *s;
    size_t len;
} string_t;

/*
 * Source code
 */
typedef struct _scode {
    char *fname;
    string_t str;
} scode_t;

/*
 * Token
 */
typedef struct _token {
    token_type_t type;
    union {
        /* Symbol, identifier, or label */
        char *sym;
        /* Prefix/Suffix keyword */
        char *fix;
        /* Numerical value */
        char *num;
    } val;
    /* Debugging information */
    scode_t *scode;
    struct {
        size_t l;
        size_t c;
    } pos;
    off_t off;
} token_t;

/*
 * An entry of token
 */
typedef struct _token_queue_entry token_queue_entry_t;
struct _token_queue_entry {
    token_t *token;
    token_queue_entry_t *next;
};

/*
 * A queue of tokens
 */
typedef struct token_queue {
    token_queue_entry_t *head;
    token_queue_entry_t *tail;
    token_queue_entry_t *cur;
} token_queue_t;

/*
 * Tokenized code (output of the first process)
 */
typedef struct tcode {
    token_queue_t *token_queue;
    /* Debugging information */
    scode_t *scode;
} tcode_t;

/*
 * Code tree
 */
typedef struct code_tree_node code_tree_node_t;
struct code_tree_node {
    tcode_t *tcode;
    size_t nr;
    code_tree_node_t *children;
};

/*
 * Preprocessed code (output of the second process)
 */
typedef struct pcode {
    token_queue_t *token_queue;
    /* Code tree */
    code_tree_node_t *tree;
} pcode_t;

/*
 * Tokenizer
 */
typedef struct tokenizer {
    scode_t *scode;
    off_t cur;
    /* Debugging information */
    struct {
        size_t l;
        size_t c;
    } pos;
} tokenizer_t;

typedef struct mvector fix_vector_t;
typedef struct _token_fix {
    char *kw;
} token_fix_t;

/*
 * Preprocessor
 */
typedef struct preprocessor {
    fix_vector_t *fix;
} preprocessor_t;


/*
 * Size prefix
 */
typedef enum _size_prefix {
    SIZE_PREFIX_NONE,
    SIZE_PREFIX_BYTE,
    SIZE_PREFIX_WORD,
    SIZE_PREFIX_DWORD,
    SIZE_PREFIX_QWORD,
} size_prefix_t;

typedef struct _prefixed_expr {
    size_prefix_t prefix;
    expr_t *expr;
} pexpr_t;

/*
 * Operand type
 */
typedef enum operand_type {
    OPERAND_ADDR_EXPR,
    OPERAND_EXPR,
} operand_type_t;

/*
 * Operand
 */
typedef struct operand {
    operand_type_t type;
    size_prefix_t prefix;
    union {
        expr_t *expr;
        pexpr_t *pexpr;
    } op;
} operand_t;
typedef struct mvector operand_vector_t;

typedef struct mvector opcode_vector_t;

/*
 * Instruction
 */
typedef struct _instruction {
    opcode_vector_t *opcode;
    operand_vector_t *operands;
} instr_t;

/*
 * Statement type
 */
typedef enum _stmt_type {
    STMT_INSTR,
    STMT_LABEL,
    STMT_GLOBAL,
} stmt_type_t;

typedef struct _stmt {
    stmt_type_t type;
    union {
        instr_t *instr;
        char *label;
        char *global;
    } u;
} stmt_t;

typedef struct mvector stmt_vector_t;




/*
 * List of instructions
 */
typedef struct vector instr_vector_t;

typedef struct vector symbol_vector_t;
typedef struct hashtable symbol_table_t;

/*
 * Assembled code
 */
typedef struct _acode {
    instr_vector_t *instrs;
    /* Symbols for linker */
    symbol_vector_t *symbols;
} acode_t;

typedef struct assembler {
    /* For linker */
    symbol_table_t *symbols;
    /* Global symbols */
    symbol_table_t *globals;
} assembler_t;





#ifdef __cplusplus
extern "C" {
#endif

    tcode_t * tokenize(preprocessor_t *, scode_t *);
    pcode_t * preprocess(preprocessor_t *, const char *);

    scode_t * scode_read(const char *);
    void scode_delete(scode_t *);

    preprocessor_t * pp_new(void);
    void pp_delete(preprocessor_t *);
    int pp_register_fix(preprocessor_t *, const char *);

    void token_queue_rewind(token_queue_t *);
    token_t * token_queue_cur(token_queue_t *);
    token_t * token_queue_next(token_queue_t *);

    void pexpr_free(pexpr_t *);

    void assemble(pcode_t *);



#ifdef __cplusplus
}
#endif

#endif /* _LAS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

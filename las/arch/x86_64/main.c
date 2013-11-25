/*_
 * Copyright 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai  <asai@scyphus.co.jp>
 */

/* $Id$ */

#include "../las.h"
#include <stdlib.h>
#include <string.h>

/*
 * x86-64 registers
 */
typedef enum op_reg {
    REG_UNKNOWN = -1,
    /* AX */
    REG_AL = 0,
    REG_AH,
    REG_AX,
    REG_EAX,
    REG_RAX,
    /* CX */
    REG_CL,
    REG_CH,
    REG_CX,
    REG_ECX,
    REG_RCX,
    /* DX */
    REG_DL,
    REG_DH,
    REG_DX,
    REG_EDX,
    REG_RDX,
    /* BX */
    REG_BL,
    REG_BH,
    REG_BX,
    REG_EBX,
    REG_RBX,
    /* SP */
    REG_SPL,
    REG_SP,
    REG_ESP,
    REG_RSP,
    /* BP */
    REG_BPL,
    REG_BP,
    REG_EBP,
    REG_RBP,
    /* SI */
    REG_SIL,
    REG_SI,
    REG_ESI,
    REG_RSI,
    /* DI */
    REG_DIL,
    REG_DI,
    REG_EDI,
    REG_RDI,
    /* R8 */
    REG_R8L,
    REG_R8W,
    REG_R8D,
    REG_R8,
    /* R9 */
    REG_R9L,
    REG_R9W,
    REG_R9D,
    REG_R9,
    /* R10 */
    REG_R10L,
    REG_R10W,
    REG_R10D,
    REG_R10,
    /* R11 */
    REG_R11L,
    REG_R11W,
    REG_R11D,
    REG_R11,
    /* R12 */
    REG_R12L,
    REG_R12W,
    REG_R12D,
    REG_R12,
    /* R13 */
    REG_R13L,
    REG_R13W,
    REG_R13D,
    REG_R13,
    /* R14 */
    REG_R14L,
    REG_R14W,
    REG_R14D,
    REG_R14,
    /* R15 */
    REG_R15L,
    REG_R15W,
    REG_R15D,
    REG_R15,
    /* Segment registers */
    REG_CS,
    REG_DS,
    REG_ES,
    REG_FS,
    REG_GS,
    /* Flags */
    REG_FLAGS,
    REG_EFLAGS,
    REG_RFLAGS,
} x86_64_reg_t;

/*
 * Pseudo instruction
 */
struct xxx {
    /* Instruction */
    instr_t *instr;
    /* Offset */
    off_t off;
    /* Estimated size */
    size_t sz;
    /* Max size */
    size_t maxsz;
};

struct y {
    char *label;
    struct xxx *ptr;
};

typedef enum _val_type {
    VAL_INT,
} val_type_t;

typedef struct _val {
    val_type_t type;
    union {
        uint64_t i;
    } u;
} val_t;

/* [base register + displacement + offset register * scalar multiplier] */
typedef struct _operand_addr {
    /* Base register */
    x86_64_reg_t base;
    /* Displacement */
    int64_t disp;
    /* Offset register */
    x86_64_reg_t offset;
    /* Scale multiplier */
    int scale;
} operand_addr_t;



/*
 * Prototype declarations
 */
static val_t * _eval_expr(expr_t *);

/*
 * Convert the register string to x86_64_reg_t
 */
static x86_64_reg_t
_strtoreg(const char *s)
{
    x86_64_reg_t reg;

    if ( 0 == strcmp("al", s) ) {
        reg = REG_AL;
    } else if ( 0 == strcmp("ah", s) ) {
        reg = REG_AH;
    } else if ( 0 == strcmp("ax", s) ) {
        reg = REG_AX;
    } else if ( 0 == strcmp("eax", s) ) {
        reg = REG_EAX;
    } else if ( 0 == strcmp("rax", s) ) {
        reg = REG_RAX;
    } else if ( 0 == strcmp("cl", s) ) {
        reg = REG_CL;
    } else if ( 0 == strcmp("ch", s) ) {
        reg = REG_CH;
    } else if ( 0 == strcmp("cx", s) ) {
        reg = REG_CX;
    } else if ( 0 == strcmp("ecx", s) ) {
        reg = REG_ECX;
    } else if ( 0 == strcmp("rcx", s) ) {
        reg = REG_RCX;
    } else if ( 0 == strcmp("dl", s) ) {
        reg = REG_DL;
    } else if ( 0 == strcmp("dh", s) ) {
        reg = REG_DH;
    } else if ( 0 == strcmp("dx", s) ) {
        reg = REG_DX;
    } else if ( 0 == strcmp("edx", s) ) {
        reg = REG_EDX;
    } else if ( 0 == strcmp("rdx", s) ) {
        reg = REG_RDX;
    } else if ( 0 == strcmp("bl", s) ) {
        reg = REG_BL;
    } else if ( 0 == strcmp("bh", s) ) {
        reg = REG_BH;
    } else if ( 0 == strcmp("bx", s) ) {
        reg = REG_BX;
    } else if ( 0 == strcmp("ebx", s) ) {
        reg = REG_EBX;
    } else if ( 0 == strcmp("rbx", s) ) {
        reg = REG_RBX;
    } else if ( 0 == strcmp("spl", s) ) {
        reg = REG_SPL;
    } else if ( 0 == strcmp("sp", s) ) {
        reg = REG_SP;
    } else if ( 0 == strcmp("esp", s) ) {
        reg = REG_ESP;
    } else if ( 0 == strcmp("rsp", s) ) {
        reg = REG_RSP;
    } else if ( 0 == strcmp("bpl", s) ) {
        reg = REG_BPL;
    } else if ( 0 == strcmp("bp", s) ) {
        reg = REG_BP;
    } else if ( 0 == strcmp("ebp", s) ) {
        reg = REG_EBP;
    } else if ( 0 == strcmp("rbp", s) ) {
        reg = REG_RBP;
    } else if ( 0 == strcmp("sil", s) ) {
        reg = REG_SIL;
    } else if ( 0 == strcmp("si", s) ) {
        reg = REG_SI;
    } else if ( 0 == strcmp("esi", s) ) {
        reg = REG_ESI;
    } else if ( 0 == strcmp("rsi", s) ) {
        reg = REG_RSI;
    } else if ( 0 == strcmp("dil", s) ) {
        reg = REG_DIL;
    } else if ( 0 == strcmp("di", s) ) {
        reg = REG_DI;
    } else if ( 0 == strcmp("edi", s) ) {
        reg = REG_EDI;
    } else if ( 0 == strcmp("rdi", s) ) {
        reg = REG_RDI;
    } else if ( 0 == strcmp("r8l", s) ) {
        reg = REG_R8L;
    } else if ( 0 == strcmp("r8w", s) ) {
        reg = REG_R8W;
    } else if ( 0 == strcmp("r8d", s) ) {
        reg = REG_R8D;
    } else if ( 0 == strcmp("r8", s) ) {
        reg = REG_R8;
    } else if ( 0 == strcmp("r9l", s) ) {
        reg = REG_R9L;
    } else if ( 0 == strcmp("r9w", s) ) {
        reg = REG_R9W;
    } else if ( 0 == strcmp("r9d", s) ) {
        reg = REG_R9D;
    } else if ( 0 == strcmp("r9", s) ) {
        reg = REG_R9;
    } else if ( 0 == strcmp("r10l", s) ) {
        reg = REG_R10L;
    } else if ( 0 == strcmp("r10w", s) ) {
        reg = REG_R10W;
    } else if ( 0 == strcmp("r10d", s) ) {
        reg = REG_R10D;
    } else if ( 0 == strcmp("r10", s) ) {
        reg = REG_R10;
    } else if ( 0 == strcmp("r11l", s) ) {
        reg = REG_R11L;
    } else if ( 0 == strcmp("r11w", s) ) {
        reg = REG_R11W;
    } else if ( 0 == strcmp("r11d", s) ) {
        reg = REG_R11D;
    } else if ( 0 == strcmp("r11", s) ) {
        reg = REG_R11;
    } else if ( 0 == strcmp("r12l", s) ) {
        reg = REG_R12L;
    } else if ( 0 == strcmp("r12w", s) ) {
        reg = REG_R12W;
    } else if ( 0 == strcmp("r12d", s) ) {
        reg = REG_R12D;
    } else if ( 0 == strcmp("r12", s) ) {
        reg = REG_R12;
    } else if ( 0 == strcmp("r13l", s) ) {
        reg = REG_R13L;
    } else if ( 0 == strcmp("r13w", s) ) {
        reg = REG_R13W;
    } else if ( 0 == strcmp("r13d", s) ) {
        reg = REG_R13D;
    } else if ( 0 == strcmp("r13", s) ) {
        reg = REG_R13;
    } else if ( 0 == strcmp("r14l", s) ) {
        reg = REG_R14L;
    } else if ( 0 == strcmp("r14w", s) ) {
        reg = REG_R14W;
    } else if ( 0 == strcmp("r14d", s) ) {
        reg = REG_R14D;
    } else if ( 0 == strcmp("r14", s) ) {
        reg = REG_R14;
    } else if ( 0 == strcmp("r15l", s) ) {
        reg = REG_R15L;
    } else if ( 0 == strcmp("r15w", s) ) {
        reg = REG_R15W;
    } else if ( 0 == strcmp("r15d", s) ) {
        reg = REG_R15D;
    } else if ( 0 == strcmp("r15", s) ) {
        reg = REG_R15;
    } else if ( 0 == strcmp("cs", s) ) {
        reg = REG_CS;
    } else if ( 0 == strcmp("ds", s) ) {
        reg = REG_DS;
    } else if ( 0 == strcmp("es", s) ) {
        reg = REG_ES;
    } else if ( 0 == strcmp("fs", s) ) {
        reg = REG_FS;
    } else if ( 0 == strcmp("gs", s) ) {
        reg = REG_GS;
    } else if ( 0 == strcmp("flags", s) ) {
        reg = REG_FLAGS;
    } else if ( 0 == strcmp("eflags", s) ) {
        reg = REG_EFLAGS;
    } else if ( 0 == strcmp("rflags", s) ) {
        reg = REG_RFLAGS;
    } else {
        reg = REG_UNKNOWN;
    }

    return reg;
}

/*
 * Evaluate var expression
 */
val_t *
_eval_expr_var(expr_t *expr)
{
    x86_64_reg_t reg;

    reg = _strtoreg(expr->u.var);
    if ( REG_UNKNOWN == reg ) {
        /* Symbol */
    } else {
        /* Register */
    }

    return NULL;
}

/*
 * Evaluate integer expression
 */
val_t *
_eval_expr_int(expr_t *expr)
{
    val_t *val;

    val = malloc(sizeof(val_t));
    if ( NULL == val ) {
        return NULL;
    }
    val->type = VAL_INT;
    val->u.i = expr->u.i;

    return val;
}

/*
 * Evaluate operand
 */
val_t *
_eval_expr_op(expr_t *expr)
{
    val_t *val;
    val_t *lval;
    val_t *rval;
    expr_t *expr0;
    expr_t *expr1;

    val = malloc(sizeof(val_t));
    if ( NULL == val ) {
        return NULL;
    }

    if ( FIX_PREFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        lval = _eval_expr(expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            /* Do nothing */
            break;
        case OP_MINUS:
            /* Minus */
            val->u.i = -lval->u.i;
            break;
        case OP_TILDE:
            /* Bit  */
            val->u.i = ~lval->u.i;
            break;
        default:
            ;
        }
    } else if ( FIX_INFIX == expr->u.op.fix_type ) {
        expr0 = vector_at(expr->u.op.args, 0);
        expr1 = vector_at(expr->u.op.args, 1);

        lval = _eval_expr(expr0);
        if ( NULL == lval ) {
            free(val);
            return NULL;
        }
        rval = _eval_expr(expr1);
        if ( NULL == rval ) {
            /* Free lval */
            free(val);
            free(lval);
            return NULL;
        }
        switch ( expr->u.op.type ) {
        case OP_PLUS:
            val->type = VAL_INT;
            val->u.i = lval->u.i + rval->u.i;
            break;
        case OP_MINUS:
            val->type = VAL_INT;
            val->u.i = lval->u.i - rval->u.i;
            break;
        case OP_MUL:
            val->type = VAL_INT;
            val->u.i = lval->u.i * rval->u.i;
            break;
        case OP_DIV:
            val->type = VAL_INT;
            val->u.i = lval->u.i / rval->u.i;
            break;
        default:
            ;
        }

    }

    return val;
}


/*
 * Evaluate the expression
 */
static val_t *
_eval_expr(expr_t *expr)
{
    val_t *val;

    switch ( expr->type ) {
    case EXPR_VAR:
        val = _eval_expr_var(expr);
        break;
    case EXPR_INT:
        val = _eval_expr_int(expr);
        break;
    case EXPR_OP:
        val = _eval_expr_op(expr);
        break;
    }

    return val;
}

/*
 * Convert expression to immediate value
 */
void
_expr_to_imm(expr_t *__restrict__ expr)
{
    val_t *val;

    switch ( expr->type ) {
    case EXPR_VAR:
        val = _eval_expr_var(expr);
        break;
    case EXPR_INT:
        val = _eval_expr_int(expr);
        break;
    case EXPR_OP:
        val = _eval_expr_op(expr);
        break;
    }
}

/*
 * MOV (Vol. 2B 4-29)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      88 /r           MOV r/m8,r8             MR      Valid   Valid
 *      REX + 88 /r     MOV r/m8***,r8***       MR      Valid   N.E.
 *      89 /r           MOV r/m16,r16           MR      Valid   Valid
 *      89 /r           MOV r/m32,r32           MR      Valid   Valid
 *      REX.W + 89 /r   MOV r/m32,r32           MR      Valid   N.E.
 *      8A /r           MOV r8,r/m8             RM      Valid   Valid
 *      REX + 8A /r     MOV r8***,r/m8***       RM      Valid   N.E.
 *      8B /r           MOV r16,r/m16           RM      Valid   Valid
 *      8B /r           MOV r32,r/m32           RM      Valid   Valid
 *      REX.W + 8B /r   MOV r64,r/m64           RM      Valid   N.E.
 *      8C /r           MOV r/m16,Sreg**        MR      Valid   Valid
 *      REX.W 8C /r     MOV r/m64,Sreg**        MR      Valid   Valid
 *      8E /r           MOV Sreg,r/m16**        RM      Valid   Valid
 *      REX.W 8E /r     MOV Sreg,r/m16**        RM      Valid   Valid
 *      A0              MOV AL,moffs8*          FD      Valid   Valid
 *      REX.W + A0      MOV AL,moffs8*          FD      Valid   N.E.
 *      A1              MOV AX,moffs16*         FD      Valid   Valid
 *      A1              MOV EAX,moffs32*        FD      Valid   Valid
 *      REX.W + A1      MOV RAX,moffs64*        FD      Valid   N.E.
 *      A2              MOV moffs8,AL           TD      Valid   Valid
 *      REX.W + A2      MOV moffs8***,AL        TD      Valid   N.E.
 *      A3              MOV moffs16*,AX         TD      Valid   Valid
 *      A3              MOV moffs32*,EAX        TD      Valid   Valid
 *      REX.W + A3      MOV moffs64*,RAX        TD      Valid   N.E.
 *      B0+ rb ib       MOV r8,imm8             OI      Valid   Valid
 *      REX + B0+ rb ib MOV r8***,imm8          OI      Valid   N.E.
 *      B8+ rw iw       MOV r16,imm16           OI      Valid   Valid
 *      B8+ rd id       MOV r32,imm32           OI      Valid   Valid
 *      REX.W + B8+ rd io
 *                      MOV r64,imm64           OI      Valid   N.E.
 *      C6 /0 ib        MOV r/m8,imm8           MI      Valid   Valid
 *      REX + C6 /0 ib  MOV r/m8***,imm8        MI      Valid   N.E.
 *      C7 /0 iw        MOV r/m16,imm16         MI      Valid   Valid
 *      C7 /0 id        MOV r/m32,imm32         MI      Valid   Valid
 *      REX.W + C7 /0 io
 *                      MOV r/m64,imm64         MI      Valid   N.E.
 *
 *      *   The moffs8, moffs16, moffs32 and moffs64 operands specify a simple
 *          offset relative to the segment base, where 8, 16, 32 and 64 refer to
 *          the size of the data.  The address-size attribute of the instruction
 *          determines the size of the offset, either 16, 32 or 64 bits.
 *      **  In 32-bit mode, the assembler may insert the 16-bit operand-size
 *          prefix with this instruction
 *      *** In 64-bit mode, AH, BH, CH, DH cannot be accessed
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      MR      ModRM:r/m(w)    ModRM:reg(r)    NA              NA
 *      RM      ModRM:reg(w)    ModRM:r/m(r)    NA              NA
 *      FD      AL/AX/EAX/RAX   Moffs           NA              NA
 *      TD      Moffs(w)        AL/AX/EAX/RAX   NA              NA
 *      OI      opcode + rd(w)  imm8/16/32/64   NA              NA
 *      MI      ModRM:r/m(w)    imm8/16/32/64   NA              NA
 */
void
_mov(operand_vector_t *operands)
{
    printf("MOV (#args = %zu)\n", mvector_size(operands));
}

/*
 * XOR (Vol. 2B 4-531)
 *
 *      Opcode          Instruction             Op/En   64-bit  Compat/Leg
 *      34 ib           XOR AL,imm8             I       Valid   Valid
 *      35 iw           XOR AX,imm16            I       Valid   Valid
 *      35 id           XOR EAX,imm32           I       Valid   Valid
 *      REX.W + 35 id   XOR RAX,imm32           I       Valid   N.E.
 *      80 /6 ib        XOR r/m8,imm8           MI      Valid   Valid
 *      REX + 80 /6 ib  XOR r/m8*,imm8          MI      Valid   N.E.
 *      81 /6 iw        XOR r/m16,imm16         MI      Valid   Valid
 *      81 /6 id        XOR r/m32,imm32         MI      Valid   Valid
 *      REX.W + 81 /6 id
 *                      XOR r/m64,imm32         MI      Valid   N.E.
 *      83 /6 ib        XOR r/m16,imm8          MI      Valid   Valid
 *      83 /6 ib        XOR r/m32,imm8          MI      Valid   Valid
 *      REX.W + 83 /6 ib
 *                      XOR r/m64,imm8          MI      Valid   N.E.
 *      30 /r           XOR r/m8,r8             MR      Valid   Valid
 *      REX + 30 /r     XOR r/m8*,r8*           MR      Valid   N.E.
 *      31 /r           XOR r/m16,r16           MR      Valid   Valid
 *      31 /r           XOR r/m32,r32           MR      Valid   Valid
 *      REX.W + 31 /r   XOR r/m64,r64           MR      Valid   N.E.
 *      32 /r           XOR r8,r/m8             RM      Valid   Valid
 *      REX + 32 /r     XOR r8*,r/m8*           RM      Valid   N.E.
 *      33 /r           XOR r16,r/m16           RM      Valid   Valid
 *      33 /r           XOR r32,r/m32           RM      Valid   Valid
 *      REX.W + 33 /r   XOR r64,r/m64           RM      Valid   N.E.
 *
 *      * In 64-bit mode, AH, BH, CH, DH cannot be accessed
 *
 *
 *      Op/En   Operand1        Operand2        Operand3        Operand4
 *      I       AL/AX/EAX/RAX   imm8/16/32      NA              NA
 *      MI      ModRM:r/m(w)    imm8/16/32      NA              NA
 *      MR      ModRM:r/m(r,w)  ModRM:reg(r)    NA              NA
 *      RM      ModRM:reg(r,w)  ModRM:r/m(r)    NA              NA
 */
void
_xor(operand_vector_t *operands)
{
    int i;
    operand_t *op;
    expr_t *expr;
    val_t *val;

    for ( i = 0; i < mvector_size(operands); i++ ) {
        op = mvector_at(operands, i);
        if ( OPERAND_ADDR_EXPR == op->type ) {
            /* Address expression */
        } else if ( OPERAND_EXPR == op->type ) {
            /* Expression */
        }
        expr = op->op.expr;
        val = _eval_expr(expr);
    }

    printf("XOR (#args = %zu)\n", mvector_size(operands));
}


/*
 * Assemble x86-64 code
 */
void
arch_assemble_x86_64(stmt_vector_t *vec)
{
    size_t i;
    stmt_t *stmt;

    for ( i = 0; i < mvector_size(vec); i++ ){
        stmt = mvector_at(vec, i);
        if ( STMT_INSTR == stmt->type ) {
            if ( 0 == strcmp("mov", stmt->u.instr->opcode) ) {
                /* MOV */
                _mov(stmt->u.instr->operands);
            } else if ( 0 == strcmp("xor", stmt->u.instr->opcode) ) {
                /* XOR */
                _xor(stmt->u.instr->operands);
            }
        }
    }

    return;
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

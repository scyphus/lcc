

# LCC: Lightweight C Compiler

## About
LCC is (to be) an implementation of lightweight C compiler with assembler and
linker.  This is mainly for my operating system but we may try to provide
portable one.  Its output format has not yet been fixed at the current time.

## LLD: Lightweight Linker


## LAS: Lightweight Assembler

### Command
    las <filename>

### Supported architectures
* x86-64 (not yet completed)

### Intel Syntax-based Assembly Language
    instruction ( operand ( "," operand )* )?


### Grammar (Intel Syntax)
    symbol ::=
                 ( letter | "_" | "." ) ( letter | digit | "_" | "." )*

    letter ::=
                 ( "A"..."Z" | "a"..."z" )

    digit ::=
                 "0"..."9"

    atom ::=
                 symbol | literal | "(" expression ")"
    
    primary ::=
                 atom
    
    u_expr ::=
                 primary | "-" u_expr | "+" u_expr | "~" u_expr
    
    m_expr ::=
                 u_expr ( ("*" | "/") u_expr )*
    
    a_expr ::=
                 m_expr ( ("+" | "-") m_expr )*
    
    shift_expr ::=
                 a_expr ( ("<<" | ">>") a_expr )*
    
    and_expr ::=
                 shift_expr ( "&" shift_expr )*
    
    xor_expr ::=
                 and_expr ( "^" and_expr )*
    
    or_expr ::=
                 xor_expr ( "|" xor_expr )*
    
    expression ::=
                 or_expr
    
    size_prefix ::=
                 ( "byte" | "word" | "dword" | "qword" )
    
    prefixed_expr ::=
                 ( size_prefix expression | expression )
    
    operand_expr ::=
                 expression ( ":" expression )?
    
    operand_addr ::=
                 "[" prefixed_expr "]"
    
    operand ::=
                 ( size_prefix )? ( operand_expr | operand_addr )
    
    operands ::=
                 operand ( "," operand )*

    instruction ::=
                 opcode ( operands )?
    
    label ::=
                 symbol ":"
    
    global ::=
                 "global" symbol
    
    input ::=
                 ( EOL | instruction | label | global )* EOF

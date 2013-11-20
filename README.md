

# LCC: Lightweight C Compiler

## About
LCC is (to be) an implementation of lightweight c compiler with assembler and
linker.  This is mainly for my operating system but we may try to provide
portable one.  Its output format has not yet been fixed at the current time.

## Grammar (Intel Syntax)
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
    
    operand_expr ::=
                 expression
    
    operand_addr ::=
                 "[" expression "]"
    
    operand ::=
                 ( operand_expr | operand_addr )
    
    instruction ::=
                 opcode operand ( "," operand )*
    
    label ::=
                 symbol ":"
    
    global ::=
                 "global" symbol
    
    input ::=
                 ( EOL | instruction | label | global )* EOF

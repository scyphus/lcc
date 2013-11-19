

# LCC: Lightweight C Compiler

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

 operand_addr ::=
              "[" expression "]"

 operand ::=
              ( operand_addr | expression )

 instruction ::=
              opcode operand ( "," operand )*

 label ::=
              symbol ":"

 global ::=
              "global" symbol

 input ::=
              ( EOL | instruction | label | global )* EOF
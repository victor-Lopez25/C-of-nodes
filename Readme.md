## Sea of nodes compiler
### WARNING: This compiler is a work in progress, it is in a very initial stage

What does this compiler include?

### frontend
```
[x] Unary, Binary Operations
[x] (some) peephole optimizations
[x] variable declarations
[x] scopes
[ ] constant definitions
[ ] functions
[ ] function pointers
[ ] macros
[ ] comptime
```

### backend
```
[ ] Obj generation
[ ] executable generation
```

## Building
1. Build `build.c` (only need to do this once):
### windows
```
gcc build.c -o build.exe
```
### other
```
gcc build.c -o build
```

2. Run build script:
```
./build [run|norun]
```

## Current language

```
<statement_list>
```
where:
```
<statement_list> ::= <statement> <statement_list> | <statement> |
<statement> ::= <return_statement> | <decl_statement> | <block_statement> | <expr_statement>
<return_statement> ::= "return" <expr> ";"
<decl_statement> ::= 
  <identifier> ":" <type> "=" <expr> |
  <identifier> ":=" <expr>           |
  <identifier> ":" <type>
<block_statement> ::= "{" <statement_list> "}"
<expr_statement> ::= <identifier> "=" <expr>

<identifier> ::= <id_first_char> <id_char> | <id_first_char>

<id_char> ::= <id_first_char> | <digit>
<id_first_char> ::= <letter> | "_"

<expr> ::= <primary> | <unary> | <binary>
<primary> ::= <number> | <identifier>

<unary> ::= <unary_op> <primary>
<binary> ::= <expr> <binary_op> <expr>

<number> ::= <digit> | <digit> <number>

<unary_op> ::= "+" | "-"
<binary_op> ::= "+" | "-" | "*" | "/"

<letter> ::= 
  "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" | 
  "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z" |
  "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | 
  "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
<digit> ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
```

## Discussion...
 - Do I want to disallow shadowing entirely, only by default or only when specified (compiler flag)?

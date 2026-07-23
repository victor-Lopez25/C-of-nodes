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
<statement>
```
where:
```
<statement> ::= "return" <expr> ";"
<expr> ::= <primary> | <unary> | <binary>
<primary> ::= <number>
<number> ::= <digit> | <digit> <number>
<digit> ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

<unary> ::= <unary_op> <primary>
<unary_op> ::= "+" | "-"

<binary> ::= <expr> <binary_op> <expr>
<binary_op> ::= "+" | "-" | "*" | "/"
```

## Discussion...
 - Do I want to disallow shadowing entirely, only by default or only when specified (compiler flag)?

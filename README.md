# toy

This is a toy bytecode compiler and interpreter for a tiny subset of
JavaScript.

The parser and the bytecode generator (in `compile.js`) are written in
toy-compatible JavaScript. You can run them with Node.js or toy
itself. They self-compile to bytecode and are magically bundled inside
`toy` (in the generated `compiler_code.c`).

## Interesting features

- Closures.
- Compiles itself (see above).
- Little CPythonish garbage collector.

## Unsupported JavaScript features

- ES201{5,6,7}
- Functions with many arguments (use arrays, dicts or curryfication
  instead)
- Protoypes
- Exceptions
- `undefined` (seriously, who need `undefined`? `null` is sufficient)
- Optional semicolons
- `for`, `switch`, `do`
- `else` (just write `if` statements. Thousands of `if` statements.)
- Regexps

## Compile and run

You need Node.js, a C compiler and GNU make. Just run `make`.

## Why?

It was fun.

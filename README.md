# toy

This is a tiny interpreter (with a bytecode compiler and a stack-based
virtual machine) for a lilliputian subset of JavaScript, written in C
and... JavaScript itself. Less than 2500 lines of code.

The parser and the bytecode generator (in `compile.js`) are written in
Toy-compatible JavaScript. It was just too painful to write those in
C.  You can run them with Node.js or Toy itself. They self-compile to
bytecode and are magically bundled inside `toy` (in the generated
`compiler_code.c`).

## Interesting features

- Closures
- Compiles itself (see above)
- Little CPythonish garbage collector

## Unsupported JavaScript features

- ES201{5,6,7}
- Functions with many arguments (use arrays, dicts or curryfication
  instead)
- Protoypes
- Exceptions
- `undefined` (seriously, who need `undefined`? `null` is sufficient)
- Optional semicolons
- Booleans (use 0 and 1, or anything else truthy or falsy)
- `for`, `switch`, `do`
- `else` (just write `if` statements. Thousands of `if` statements.)
- Regexps

## Compile and run

You need Node.js[^node], a C compiler and GNU make. Just run `make` and try
the examples.

## Why?

It was fun.

[^node]: Actually, you don’t even need Node.js to compile Toy since
the bytecode is architecture-independent. You can perfecly use a
`compiler_code.c` generated on another machine.

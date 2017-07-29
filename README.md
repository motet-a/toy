# toy

This is a tiny interpreter (with a bytecode compiler and a stack-based
virtual machine) for a lilliputian subset of JavaScript, written in C
and... JavaScript itself. It fits in less than 2500 lines of code.

The parser and the bytecode generator (in `compile.js`) are written in
Toy-compatible JavaScript. It was just too painful to write those in
C.  Since you can run them with Node.js or Toy itself, they
self-compile (into `compiler_code.c`) and are magically bundled inside
the `toy` executable.

## Interesting features

- Closures
- Compiles itself (see above)
- Little mark-and-sweep CPythonish garbage collector

## Unsupported JavaScript features

- ES201{5,6,7}
- Functions with many arguments (use lists, dictionnaries or
  curryfication instead)
- Prototypes
- Exceptions
- `undefined` (seriously, who needs `undefined`? `null` is sufficient)
- Optional semicolons
- Booleans (use 0 and 1, or anything else truthy or falsy)
- `for`, `switch`, `do`
- `else` (just write `if` statements. Thousands of `if` statements.)
- Regexps

## Compile and run

You need Node.js, a C compiler and GNU make. Just run `make` and try
the examples.

## Various observations

First of all, because I hate naming things _à la_ JavaScript:

  - an _object_, a _hash map_ or a _hash table_ is _dictionnary_
  - an _array_ is a _list_

For the sake of simplicity, the whole thing is really slow.

The dictionnary implementation is a shame.

Lists are implemented with those dictionnaries. This is really
straightforward since we need dictionnaries and JavaScript lists
behaves mostly the same. It’s obviously slow.

Since the garbage collector does not visit the stack, each object has
a reference counter which prevents it from being collected if that
counter is nonzero. Moreover, the GC must not run at any time, but
only between two instructions (see the call to
`request_garbage_collection();` in `vm.c`).

Variables are compiled as-is. The VM interprets scoping rules and
catches variable definition errors at run-time. The compiler is really
straightforward.

I just hope you are not crazy enough to use this hack in production.

## Why?

It was fun. Enjoy :wink:

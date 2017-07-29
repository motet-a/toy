#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "value.h"

enum opcode {
#define X(name) opcode_ ## name,
#  include "opcode.def"
#undef X
};

typedef struct compiled_func compiled_func_t;
typedef struct compiled_file compiled_file_t;

struct compiled_func {
    char *param_name; // may be null
    unsigned char *code;
    value_t *consts;
    struct bvalue *bconsts;
    size_t const_count;
    compiled_file_t *file;
};

struct compiled_file {
    compiled_func_t *funcs;
    size_t func_count;
};

value_t call_func(value_t func, value_t arg);

// The optional `<parent>` property of the scope must be set
value_t eval_func(value_t func, value_t scope);

// Returns -1 on error
enum opcode string_to_opcode(const char *s);

#endif /* VM_H */

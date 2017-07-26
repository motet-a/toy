#ifndef OBJECT_H
#define OBJECT_H

#include "dict.h"

typedef struct object object_t;
typedef struct value value_t;
typedef struct func func_t;

typedef value_t (*native_func_t)(value_t parent_scope, value_t arg);

enum object_type {
    object_type_dict,
    object_type_list,
    object_type_string,
    object_type_func,
};

struct func {
    struct compiled_func *compiled; // null if native
    native_func_t native;  // null if not native

    // If the function is compiled, the parent_scope must be a dict
    // with an optional `<parent>` property.
    // If the function is native, it is user-defined and can be anything.
    value_t parent_scope;
};

struct object {
    enum object_type type;
    object_t *prev, *next; // Garbage collection junk
    int marked, ref_count; // More garbage collection junk
    union {
        dict_t dict; // This is also used for lists. No, I’m not kidding.
        char *string;
        func_t func;
    };
};

void free_object_unsafe(object_t *o);
object_t *new_string_object(const char *cs);
object_t *new_dict_object(void);
object_t *new_list_object(void);
object_t *new_native_func_object(native_func_t func);
object_t *new_compiled_func_object(struct compiled_func *compiled);

// These global variables are used by the garbage collector.
extern object_t *big_linked_list; // Contains every allocated object.
extern unsigned long object_count;
extern unsigned long allocation_count_since_last_gc;

#endif /* OBJECT_H */

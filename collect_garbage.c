#include "toy.h"

static void mark_object(object_t *object);
static void mark_compiled_func(const struct compiled_func *cf);

static void mark_value(value_t v) {
    if (v_is_object(v)) {
        mark_object(v.object);
    }
}

static void mark_dict(dict_t *dict) {
    for (dict_entry_t *e = *dict; e; e = e->next) {
        mark_value(e->value);
    }
}

static void mark_compiled_file(const struct compiled_file *cf) {
    for (size_t i = 0; i < cf->func_count; i++) {
        mark_compiled_func(cf->funcs + i);
    }
}

static void mark_compiled_func(const struct compiled_func *cf) {
    for (size_t i = 0; i < cf->const_count; i++) {
        mark_value(cf->consts[i]);
    }
}

static void mark_func(object_t *object) {
    mark_value(object->func.parent_scope);
    const struct compiled_func *cf = object->func.compiled;
    if (cf) {
        mark_compiled_func(cf);
        mark_compiled_file(cf->file);
    }
}

static void mark_object(object_t *object) {
    if (object->marked) {
        return;
    }

    object->marked = 1;
    switch (object->type) {
    case object_type_list:
    case object_type_dict:
        mark_dict(&object->dict);
        break;
    case object_type_func: {
        mark_func(object);
        break;
    }
    default:
        break;
    }
}

void collect_garbage(void) {
    for (object_t *o = big_linked_list; o; o = o->next) {
        o->marked = 0;
    }

    for (object_t *o = big_linked_list; o; o = o->next) {
        if (o->ref_count) {
            mark_object(o);
        }
    }

    object_t *o = big_linked_list;
    while (o) {
        object_t *next = o->next;
        if (!o->marked) {
            free_object_unsafe(o);
        }
        o = next;
    }
}

static unsigned long object_count_after_last_gc = 0;

void request_garbage_collection(void) {
    if (allocation_count_since_last_gc > object_count_after_last_gc) {
        collect_garbage();
        allocation_count_since_last_gc = 0;
        object_count_after_last_gc = object_count;
    }
}

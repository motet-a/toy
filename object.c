#include "toy.h"

unsigned long object_count = 0;
unsigned long allocation_count_since_last_gc = 0;
object_t *big_linked_list = 0;

static object_t *new_object(void) {
    object_count++;
    allocation_count_since_last_gc++;
    object_t *o = xmalloc(sizeof(object_t));
    o->ref_count = 0;
    o->prev = 0;
    o->next = big_linked_list;
    if (big_linked_list) {
        big_linked_list->prev = o;
    }
    big_linked_list = o;
    return o;
}

void free_object_unsafe(object_t *o) {
    if (o == big_linked_list) {
        big_linked_list = o->next;
    }
    if (o->prev) {
        o->prev->next = o->next;
    }
    if (o->next) {
        o->next->prev = o->prev;
    }

    switch (o->type) {
    case object_type_list: // fallthrough
    case object_type_dict:
        dict_delete_all(&o->dict);
        break;
    case object_type_string:
        free(o->string);
        break;
    case object_type_func:
        break;
    }
    free(o);
    object_count--;
}

object_t *new_string_object(const char *cs) {
    object_t *o = new_object();
    o->type = object_type_string;
    o->string = xstrdup(cs);
    return o;
}

object_t *new_dict_object(void) {
    object_t *o = new_object();
    o->type = object_type_dict;
    memset(&o->dict, 0, sizeof(dict_t));
    return o;
}

object_t *new_list_object(void) {
    object_t *o = new_dict_object();
    o->type = object_type_list;
    dict_set(&o->dict, "length", v_number(0));
    return o;
}

static object_t *new_func_object(func_t func) {
    object_t *o = new_object();
    o->type = object_type_func;
    o->func = func;
    return o;
}

object_t *new_native_func_object(native_func_t native) {
    return new_func_object((func_t){
        .compiled = 0,
        .native = native,
        .parent_scope = v_null,
    });
}

object_t *new_compiled_func_object(compiled_func_t *compiled) {
    return new_func_object((func_t){
        .compiled = compiled,
        .native = 0,
        .parent_scope = v_null,
    });
}

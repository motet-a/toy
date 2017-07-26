#include "toy.h"
#include <stdarg.h>

const value_t v_null = {.type = value_type_null};

int v_to_bool(value_t v) {
    return v.type == value_type_number ? !!v.number :
        v.type == value_type_null ? 0 :
        v_is_string(v) ? strlen(v.object->string) :
        1;
}

char *v_to_string(value_t v) {
    switch (v.type) {
    case value_type_number: {
        char buf[64];
        snprintf(buf, 64, "%g", v.number);
        return xstrdup(buf);
    }

    case value_type_null:
        return xstrdup("null");

    case value_type_object:
        switch (v.object->type) {
        case object_type_dict: return xstrdup("[dict]");
        case object_type_list: return xstrdup("[list]");
        case object_type_func: return xstrdup("[function]");
        case object_type_string: return xstrdup(v.object->string);
        }
    }
    abort();
}

double v_to_number(value_t v) {
    if (v_is_number(v)) {
        return v.number;
    }
    if (v_is_string(v)) {
        double n;
        if (sscanf(v.object->string, "%lf", &n) == 1) {
            return n;
        }
    }
    return 0;
}

long v_to_integer(value_t v) {
    double n = v_to_number(v);
    if (n >= MAX_SAFE_INTEGER) {
        n = MAX_SAFE_INTEGER;
    }
    if (n <= MIN_SAFE_INTEGER) {
        n = MIN_SAFE_INTEGER;
    }
    return n;
}

value_t v_add(value_t a, value_t b) {
    if (v_is_number(a) && v_is_number(b)) {
        return v_number(a.number + b.number);
    }

    char *left = v_to_string(a);
    char *right = v_to_string(b);
    char *s = xmalloc(strlen(left) + strlen(right) + 1);
    strcpy(s, left);
    strcat(s, right);
    free(left);
    free(right);
    value_t result = v_string(s);
    free(s);
    return result;
}

// Binary operations on numbers (except modulo, which requires integers)
#define X(name, op)                                   \
    value_t v_##name(value_t a, value_t b) {          \
        if (v_is_number(a) && v_is_number(b)) {       \
            return v_number(a.number op b.number);    \
        }                                             \
        return v_number(0);                           \
    }
X(sub, -) X(mul, *) X(div, /) X(gt, >) X(lt, <) X(gte, >=) X(lte, <=)
#undef X

value_t v_mod(value_t a, value_t b) {
    if (v_is_number(a) && v_is_number(b)) {
        return v_number(v_to_integer(a) % v_to_integer(b));
    }
    return v_number(0);
}

static int object_equal(const object_t *a, const object_t *b) {
    return a->type != b->type ? 0 :
        a->type == object_type_string ? strcmp(a->string, b->string) == 0 :
        0;
}

int v_equal(value_t a, value_t b) {
    return a.type != b.type ? 0 :
        a.type == value_type_number ? a.number == b.number :
        a.type == value_type_null ? 1 :
        object_equal(a.object, b.object);
}

const char *v_typeof(value_t a) {
    switch (a.type) {
    case value_type_null: return "null";
    case value_type_number: return "number";
    case value_type_object:
        switch (a.object->type) {
        case object_type_string: return "string";
        case object_type_func: return "function";
        case object_type_list: return "list";
        default: return "object";
        }
    }
    abort();
}

void v_set(value_t dict, value_t key, value_t v) {
    if (v_is_dict(dict)) {
        dict_setv(&dict.object->dict, key, v);
    } else if (v_is_list(dict)) {
        size_t index = v_to_integer(key);
        if (index < v_list_length(dict)) {
            return dict_setv(&dict.object->dict, key, v);
        }
    }
}

static value_t string_slice(value_t vstring, value_t vindex) {
    v_assert_type(vstring, string);
    size_t index = v_to_integer(vindex);
    const char *string = vstring.object->string;
    size_t len = strlen(string);
    if (index >= len) {
        return v_string("");
    }
    size_t new_len = len - index;
    char *new = malloc(new_len + 1);
    memcpy(new, string + index, new_len);
    new[new_len] = 0;
    value_t vnew = v_string(new);
    free(new);
    return vnew;
}

static value_t string_index_of(value_t vstring, value_t vneedle) {
    v_assert_type(vneedle, string);
    v_assert_type(vstring, string);

    const char *s = vstring.object->string;
    char *begin = strstr(s, vneedle.object->string);
    return begin ? v_number(begin - s) : v_number(-1);
}

static value_t string_char_code_at(value_t vstring, value_t index) {
    v_assert_type(vstring, string);
    size_t i = v_to_integer(index);
    const char *s = vstring.object->string;
    return i < strlen(s) ? v_number(s[i]) : v_null;
}

value_t v_in(value_t key, value_t dict) {
    return v_number((v_is_dict(dict) || v_is_list(dict)) &&
                    dict_hasv(&dict.object->dict, key));
}

size_t v_list_length(value_t list) {
    v_assert_type(list, list);
    return v_to_integer(dict_get(&list.object->dict, "length"));
}

value_t v_list_push(value_t list, value_t new) {
    v_assert_type(list, list);
    size_t index = v_list_length(list);
    dict_setv(&list.object->dict, v_number(index), new);
    dict_set(&list.object->dict, "length", v_number(index + 1));
    return v_null;
}

static value_t v_list_concat(value_t a, value_t b) {
    v_assert_type(a, list); v_assert_type(b, list);
    value_t new = v_list();
    size_t length = v_list_length(a), i;
    for (i = 0; i < length; i++) {
        v_list_push(new, v_get(a, v_number(i)));
    }
    length = v_list_length(b);
    for (i = 0; i < length; i++) {
        v_list_push(new, v_get(b, v_number(i)));
    }
    return new;
}

static value_t v_list_index_of(value_t list, value_t item) {
    v_assert_type(list, list);
    size_t length = v_list_length(list);
    for (size_t i = 0; i < length; i++) {
        if (v_equal(v_get(list, v_number(i)), item)) {
            return v_number(i);
        }
    }
    return v_number(-1);
}

static value_t create_method(value_t object, native_func_t func) {
    value_t m = v_native_func(func);
    m.object->func.parent_scope = object;
    return m;
}

static value_t get_list_property(value_t list, const char *key) {
    if (strcmp(key, "length") == 0) {
        return dict_get(&list.object->dict, "length");
    }
    if (strcmp(key, "indexOf") == 0) {
        return create_method(list, v_list_index_of);
    }
    if (strcmp(key, "push") == 0) {
        return create_method(list, v_list_push);
    }
    if (strcmp(key, "concat") == 0) {
        return create_method(list, v_list_concat);
    }

    return v_null;
}

static value_t get_string_property(value_t string, const char *key) {
    if (strcmp(key, "length") == 0) {
        return v_number(strlen(string.object->string));
    }
    if (strcmp(key, "slice") == 0) {
        return create_method(string, string_slice);
    }
    if (strcmp(key, "indexOf") == 0) {
        return create_method(string, string_index_of);
    }
    if (strcmp(key, "charCodeAt") == 0) {
        return create_method(string, string_char_code_at);
    }

    return v_null;
}

value_t v_get(value_t obj, value_t key) {
    if (v_is_dict(obj)) {
        return dict_getv(&obj.object->dict, key);

    } else if (v_is_list(obj)) {
        if (v_is_number(key)) {
            size_t index = v_to_integer(key);
            if (index < v_list_length(obj)) {
                return dict_getv(&obj.object->dict, key);
            }
        }

        char *skey = v_to_string(key);
        value_t result = get_list_property(obj, skey);
        free(skey);
        return result;

    } else if (v_is_string(obj)) {
        if (v_is_number(key)) {
            size_t index = key.number;
            const char *s = obj.object->string;
            if (index < strlen(s)) {
                char c[2] = {s[index], 0};
                return v_string(c);
            }
            return v_null;
        }

        char *skey = v_to_string(key);
        value_t result = get_string_property(obj, skey);
        free(skey);
        return result;
    }

    return v_null;
}

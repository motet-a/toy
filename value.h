#ifndef VALUE_H
#define VALUE_H

#include <math.h>
#include <stddef.h>

#define MAX_SAFE_INTEGER (9007199254740991)
#define MIN_SAFE_INTEGER (-9007199254740991)

typedef struct object object_t;
typedef struct value value_t;

enum value_type {
    value_type_null,
    value_type_number,
    value_type_object,
};

struct value {
    enum value_type type;
    union {
        object_t *object;
        double number; // NaN is invalid
    };
};

extern const value_t v_null;

static inline value_t v_number(double nbr) {
    return (value_t){
        .type = value_type_number,
        .number = isnan((double)nbr) ? 0 : nbr
    };
}

#define v_native_func(f)                        \
    ((value_t){                                 \
        .type = value_type_object,              \
        .object = new_native_func_object(f),    \
    })

#define v_string(cstr)                          \
    ((value_t){                                 \
        .type = value_type_object,              \
        .object = new_string_object(cstr),      \
    })

#define v_string_from_char(c)                   \
    (v_string((char[]){c}))

#define v_dict()                                \
    ((value_t){                                 \
        .type = value_type_object,              \
        .object = new_dict_object(),            \
    })

#define v_list()                                \
    ((value_t){                                 \
        .type = value_type_object,              \
        .object = new_list_object(),            \
    })

#define v_is_object(v)  ((v).type == value_type_object)

#define v_is_object_of_type(v, expected_type)        \
    (v_is_object(v) &&                               \
     (v).object->type == object_type_##expected_type)

#define v_is_null(v)    ((v).type == value_type_null)
#define v_is_number(v)  ((v).type == value_type_number)
#define v_is_dict(v)    (v_is_object_of_type((v), dict))
#define v_is_list(v)    (v_is_object_of_type((v), list))
#define v_is_string(v)  (v_is_object_of_type((v), string))
#define v_is_func(v)    (v_is_object_of_type((v), func))

// Used to force the GC not to collect an object (because the GC does not
// visit the C stack).
#define v_inc_ref(v)                                    \
    ({                                                  \
        __auto_type inc_ref__v = (v);                   \
        if ((inc_ref__v).type == value_type_object) {   \
            inc_ref__v.object->ref_count++;             \
        }                                               \
    })

#define v_dec_ref(v)                                    \
    ({                                                  \
        __auto_type dec_ref__v = (v);                   \
        if ((dec_ref__v).type == value_type_object) {   \
            dec_ref__v.object->ref_count--;             \
        }                                               \
    })

#define v_assert_type(v, type)                  \
    if (!v_is_##type(v)) {                      \
        die("must be a " #type);                \
    }

int v_to_bool(value_t v);
char *v_to_string(value_t v);
double v_to_number(value_t v);
long v_to_integer(value_t v);

// Binary operators
#define X(name) value_t v_##name(value_t a, value_t b);
    X(add) X(sub) X(mul) X(div) X(mod) X(gt) X(lt) X(gte) X(lte)
#undef X

const char *v_typeof(value_t a);

int v_equal(value_t a, value_t b);
#define v_eq(a, b)  (v_number(v_equal(a, b)))
#define v_neq(a, b) (v_number(!v_equal(a, b)))

value_t v_in(value_t key, value_t dict);
void v_set(value_t dict, value_t key, value_t v);
value_t v_get(value_t dict, value_t key);

value_t v_list_push(value_t list, value_t new);
size_t v_list_length(value_t list);

#endif /* VALUE_H */

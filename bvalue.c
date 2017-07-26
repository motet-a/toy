#include "toy.h"
#include "bvalue.h"

static value_t bvalue_to_v(bvalue_t b) {
    switch (b.type) {
    case bvalue_type_string: return v_string(b.string);
    case bvalue_type_number: return v_number(b.number);
    }
    abort();
}

void bvalue_array_to_v(value_t *v, const bvalue_t *b, size_t length) {
    for (size_t i = 0; i < length; i++) {
        v[i] = bvalue_to_v(b[i]);
    }
}

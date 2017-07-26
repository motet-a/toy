#ifndef BUILTIN_H
#define BUILTIN_H

#include "value.h"

typedef struct bvalue bvalue_t;

enum bvalue_type {
    bvalue_type_string,
    bvalue_type_number,
};

struct bvalue {
    enum bvalue_type type;
    union {
        const char *string;
        double number;
    };
};

#define bv_nbr(n)                               \
    {                                           \
        .type = bvalue_type_number,             \
        .number = (n),                          \
    }

#define bv_str(s)                               \
    {                                           \
        .type = bvalue_type_string,             \
        .string = (s),                          \
    }

void bvalue_array_to_v(value_t *v, const bvalue_t *b, size_t length);

#endif /* BUILTIN_H */

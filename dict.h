#ifndef DICT_H
#define DICT_H

#include "value.h"

typedef struct dict_entry dict_entry_t;
typedef dict_entry_t *dict_t; // A pointer to the first entry

struct dict_entry {
    char *key;
    value_t value;
    dict_entry_t *prev, *next;
};

value_t dict_get(const dict_t *dict, const char *key);
value_t dict_getv(const dict_t *dict, value_t key);
int dict_has(const dict_t *dict, const char *key);
int dict_hasv(const dict_t *dict, value_t key);
void dict_set(dict_t *dict, const char *key, value_t v);
void dict_setv(dict_t *dict, value_t key, value_t v);
void dict_delete_all(dict_t *dict);

#endif /* DICT_H */

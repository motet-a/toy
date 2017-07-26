#include "toy.h"

// Here is how *not* to implement a dictionnary.
// (Do you want to know a secret? This is also used for the lists.)

static dict_entry_t *dict_find_entry(dict_t *dict, const char *key) {
    for (dict_entry_t *e = *dict; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            return e;
        }
    }
    return 0;
}

value_t dict_get(const dict_t *dict, const char *key) {
    const dict_entry_t *entry = dict_find_entry((dict_t *)dict, key);
    return entry ? entry->value : v_null;
}

value_t dict_getv(const dict_t *dict, value_t key) {
    char *skey = v_to_string(key);
    value_t v = dict_get(dict, skey);
    free(skey);
    return v;
}

int dict_has(const dict_t *dict, const char *key) {
    return !!dict_find_entry((dict_t *)dict, key);
}

int dict_hasv(const dict_t *dict, value_t key) {
    char *skey = v_to_string(key);
    int ok = dict_has(dict, skey);
    free(skey);
    return ok;
}

void dict_set(dict_t *dict, const char *key, value_t v) {
    dict_entry_t *entry = dict_find_entry(dict, key);
    if (entry) {
        entry->value = v;
        return;
    }
    entry = xmalloc(sizeof(dict_entry_t));
    entry->key = xstrdup(key);
    entry->value = v;
    entry->next = *dict;
    entry->prev = 0;
    if (*dict) {
        (*dict)->prev = entry;
    }
    *dict = entry;
}

void dict_setv(dict_t *dict, value_t key, value_t v) {
    char *skey = v_to_string(key);
    dict_set(dict, skey, v);
    free(skey);
}

void dict_delete_all(dict_t *dict) {
    dict_entry_t *e = *dict;
    while (e) {
        dict_entry_t *next = e->next;
        free(e->key);
        free(e);
        e = next;
    }
}

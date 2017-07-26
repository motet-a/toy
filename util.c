#include "toy.h"

#define ASSERT_ENOUGH_MEM(v)                    \
    if (!v) {                                   \
        die("cannot allocate memory");          \
    }

void die(const char *error) {
    fprintf(stderr, "fatal: %s\n", error);
    exit(1);
}

void *xmalloc(size_t size) {
    void *d = malloc(size);
    ASSERT_ENOUGH_MEM(d);
    return d;
}

char *xstrdup(const char *s) {
    char *r = strdup(s);
    ASSERT_ENOUGH_MEM(r);
    return r;
}

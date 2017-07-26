#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

__attribute__((noreturn)) void die(const char *error);
void *xmalloc(size_t size);
char *xstrdup(const char *s);

#endif /* UTIL_H */

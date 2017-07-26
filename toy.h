#ifndef TOY_H
#define TOY_H

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "vm.h"
#include "value.h"
#include "util.h"

value_t eval_source(const char *source);
void collect_garbage(void);
void request_garbage_collection(void);
struct compiled_file *get_builtin_file(void);

#endif /* TOY_H */

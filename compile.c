#include "toy.h"
#include <errno.h>

static value_t v_die(value_t ctx, value_t message) {
    (void)ctx;
    die(v_to_string(message));
}

static value_t v_print(value_t ctx, value_t message) {
    (void)ctx;
    char *s = v_to_string(message);
    puts(s);
    free(s);
    return v_null;
}

static value_t v_parse_int(value_t ctx, value_t vstring) {
    (void)ctx;
    char *endptr;
    char *s = v_to_string(vstring);
    errno = 0;
    long number = strtol(s, &endptr, 10);
    if (errno || *endptr) {
        free(s);
        return v_null;
    }
    free(s);
    return v_number(number);
}

static value_t v_math_floor(value_t ctx, value_t number) {
    (void)ctx;
    return v_number(v_to_integer(number));
}

static value_t get_math(void) {
    value_t m = v_dict();
    v_set(m, v_string("floor"), v_native_func(v_math_floor));
    return m;
}

static value_t get_global_scope(void) {
    value_t scope = v_dict();
    v_set(scope, v_string("die"), v_native_func(v_die));
    v_set(scope, v_string("print"), v_native_func(v_print));
    v_set(scope, v_string("parseInt"), v_native_func(v_parse_int));
    v_set(scope, v_string("Math"), get_math());
    return scope;
}

static value_t import_nodejs_module(value_t file_func, value_t global_scope) {
    value_t exports = v_dict();
    value_t module = v_dict();
    v_set(module, v_string("exports"), exports);
    v_set(global_scope, v_string("module"), module);
    eval_func(file_func, global_scope);
    return v_get(module, v_string("exports"));
}

static value_t get_builtin_file_func(void) {
    compiled_file_t *file = get_builtin_file();
    value_t vglobal = {
        .type = value_type_object,
        .object = new_compiled_func_object(file->funcs),
    };
    return vglobal;
}

static value_t import_builtin_compiler_module(void) {
    return import_nodejs_module(get_builtin_file_func(), get_global_scope());
}

static compiled_func_t translate_compiled_func(value_t vfunc) {
    value_t vcode = v_get(vfunc, v_string("code"));
    value_t vconsts = v_get(vfunc, v_string("consts"));
    size_t code_length = v_list_length(vcode);
    unsigned char *code = xmalloc(code_length);
    size_t const_count = v_list_length(vconsts);
    value_t *consts = xmalloc(sizeof(value_t) * const_count);

    for (size_t i = 0; i < const_count; i++) {
        consts[i] = v_get(vconsts, v_number(i));
    }

    for (size_t i = 0; i < code_length; i++) {
        value_t vinstr = v_get(vcode, v_number(i));
        if (v_is_string(vinstr)) {
            int opcode = string_to_opcode(vinstr.object->string);
            if (opcode == -1) {
                die("unknown opcode");
            }
            code[i] = opcode;
        } else {
            code[i] = v_to_integer(vinstr);
        }
    }

    return (compiled_func_t){
        .param_name = v_to_string(v_get(vfunc, v_string("paramName"))),
        .code = code,
        .consts = consts,
        .const_count = const_count,
    };
}

static compiled_file_t *translate_compiled_file(value_t vfuncs) {
    compiled_file_t *file = xmalloc(sizeof(compiled_file_t));
    file->func_count = v_list_length(vfuncs);
    file->funcs = xmalloc(sizeof(compiled_func_t) * file->func_count);
    for (size_t i = 0; i < file->func_count; i++) {
        value_t vfunc = v_get(vfuncs, v_number(i));
        compiled_func_t func = translate_compiled_func(vfunc);
        func.file = file;
        memcpy(file->funcs + i, &func, sizeof(compiled_func_t));
    }
    return file;
}

static void free_compiled_file(compiled_file_t *file) {
    for (size_t i = 0; i < file->func_count; i++) {
        compiled_func_t *func = file->funcs + i;
        free(func->code);
        free(func->consts);
        free(func->param_name);
    }
    free(file->funcs);
    free(file);
}

value_t eval_source(const char *source) {
    value_t compile_func = import_builtin_compiler_module();
    if (!v_is_func(compile_func)) {
        die("the builtin compiler module must export a function");
    }

    value_t compiled_funcs = call_func(compile_func, v_string(source));
    compiled_file_t *file = translate_compiled_file(compiled_funcs);
    value_t func = {
        .type = value_type_object,
        .object = new_compiled_func_object(file->funcs),
    };
    value_t result = eval_func(func, get_global_scope());
    free_compiled_file(file);
    collect_garbage();
    return result;
}

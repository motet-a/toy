#include "toy.h"

static value_t scope_lookup(value_t scope, const char *name) {
    if (dict_has(&scope.object->dict, name)) {
        return scope;
    }
    value_t parent = dict_get(&scope.object->dict, "<parent>");
    if (!v_is_null(parent)) {
        return scope_lookup(parent, name);
    }
    return v_null;
}

static value_t scope_lookup_or_die(value_t scope, const char *name) {
    scope = scope_lookup(scope, name);
    if (!v_is_null(scope)) {
        return scope;
    }
    value_t namev = v_string(name);
    die(v_to_string(v_add(v_string("undefined variable "), namev)));
}

static value_t scope_get(value_t scope, const char *name) {
    scope = scope_lookup_or_die(scope, name);
    return dict_get(&scope.object->dict, name);
}

static void scope_set(value_t scope, const char *name, value_t v) {
    scope = scope_lookup_or_die(scope, name);
    dict_set(&scope.object->dict, name, v);
}

static void scope_decl(value_t scope, const char *name) {
    if (!dict_has(&scope.object->dict, name)) {
        dict_set(&scope.object->dict, name, v_null);
    }
}

value_t call_func(value_t func, value_t arg) {
    v_inc_ref(arg);
    v_inc_ref(func);
    if (!v_is_func(func)) {
        die("call_func(): not a function");
    }
    value_t parent_scope = func.object->func.parent_scope;
    compiled_func_t *compiled = func.object->func.compiled;
    value_t result;
    if (compiled) {
        value_t child_scope = v_dict();
        v_inc_ref(child_scope);
        dict_set(&child_scope.object->dict, "<parent>", parent_scope);
        if (compiled->param_name) {
            dict_set(&child_scope.object->dict, compiled->param_name, arg);
        }
        result = eval_func(func, child_scope);
        v_dec_ref(child_scope);
    } else {
        result = func.object->func.native(parent_scope, arg);
    }
    v_dec_ref(arg);
    v_dec_ref(func);
    return result;
}

typedef struct stackk stackk_t;

#define STACK_CAPACITY 20

struct stackk {
    value_t list[STACK_CAPACITY];
    size_t size;
};

static value_t stack_pop(stackk_t *stack) {
    if (!stack->size) {
        die("stack underflow");
    }
    value_t value = stack->list[--(stack->size)];
    v_dec_ref(value);
    return value;
}

static value_t stack_get_top(const stackk_t *stack) {
    if (!stack->size) {
        die("empty stack");
    }
    return stack->list[stack->size - 1];
}

static void stack_push(stackk_t *stack, value_t value) {
    if (stack->size == STACK_CAPACITY) {
        die("stack overflow");
    }
    v_inc_ref(value);
    stack->list[stack->size++] = value;
}

static void stack_flush(stackk_t * stack) {
    for (size_t i = 0; i < stack->size; i++) {
        v_dec_ref(stack->list[i]);
    }
}

value_t eval_func(value_t funcv, value_t scope) {
    v_assert_type(funcv, func);
    v_inc_ref(funcv);
    v_inc_ref(scope);
    func_t *func = &funcv.object->func;
    const compiled_func_t *comp = func->compiled;
    stackk_t stack = {};
    size_t ip = 0;

#define tos (stack_get_top(&stack))

#define push(v) stack_push(&stack, (v))
#define pop()   stack_pop(&stack)

#define peek_opcode(offset) (comp->code[ip + (offset)])

#define next_opcode()                           \
    ({                                          \
        enum opcode next__op = peek_opcode(0);  \
        ip++;                                   \
        next__op;                               \
    })

    // Big endian
#define peek_uint16()                                   \
    ((unsigned)peek_opcode(0) * 0x100 + peek_opcode(1)) \

    for (;;) {
        request_garbage_collection();

        enum opcode opcode = next_opcode();
        switch (opcode) {
        case opcode_return:
            v_dec_ref(funcv);
            v_dec_ref(scope);
            stack_flush(&stack);
            if (!stack.size) {
                return v_null;
            }
            return tos;

        case opcode_load_const: {
            unsigned index = peek_uint16();
            ip += 2;
            if (index >= comp->const_count) {
                die("load_const: const index out of range");
            }
            push(comp->consts[index]);
            break;
        }

        case opcode_load_null:
            push(v_null);
            break;

        case opcode_load_empty_list:
            push(v_list());
            break;

        case opcode_load_empty_dict:
            push(v_dict());
            break;

        case opcode_list_push: {
            value_t item = pop();
            value_t list = tos;
            v_assert_type(list, list);
            v_list_push(list, item);
            break;
        }

        case opcode_dict_push: {
            value_t value = pop();
            value_t key = pop();
            value_t dict = tos;
            v_set(dict, key, value);
            break;
        }

        case opcode_pop:
            pop();
            break;

        case opcode_decl_var: {
            value_t vname = pop();
            v_assert_type(vname, string);
            scope_decl(scope, vname.object->string);
            break;
        }

        case opcode_load_var: {
            value_t vname = pop();
            v_assert_type(vname, string);
            push(scope_get(scope, vname.object->string));
            break;
        }

        case opcode_load_func: {
            unsigned index = peek_uint16();
            ip += 2;
            if (index >= comp->file->func_count) {
                die("load_func: func index out of range");
            }
            compiled_func_t *comp_closure = comp->file->funcs + index;
            object_t *closure_obj = new_compiled_func_object(comp_closure);
            closure_obj->func.parent_scope = scope;
            value_t closure_value = {
                .type = value_type_object,
                .object = closure_obj,
            };
            push(closure_value);
            break;
        }

        case opcode_call: {
            value_t arg = pop();
            value_t child_func = pop();
            push(call_func(child_func, arg));
            break;
        }

        case opcode_store_var: {
            value_t value = pop();
            value_t vname = pop();
            v_assert_type(vname, string);
            scope_set(scope, vname.object->string, value);
            break;
        }

        case opcode_dup:
            push(tos);
            break;

        case opcode_goto:
            ip = peek_uint16();
            break;

        case opcode_goto_if: {
            unsigned next = peek_uint16();
            ip += 2;
            if (v_to_bool(pop())) {
                ip = next;
            }
            break;
        }

        case opcode_not:
            push(v_number(!v_to_bool(pop())));
            break;

        case opcode_unary_minus:
            push(v_number(-v_to_number(pop())));
            break;

        case opcode_typeof:
            push(v_string(v_typeof(pop())));
            break;

        case opcode_set: {
            value_t key = pop();
            value_t dict = pop();
            value_t new_value = pop();
            v_set(dict, key, new_value);
            break;
        }

        case opcode_get: {
            value_t key = pop();
            value_t dict = pop();
            push(v_get(dict, key));
            break;
        }

        case opcode_rot: {
            value_t a = pop();
            value_t b = pop();
            push(a);
            push(b);
            break;
        }

#define case_bin_op(name)                       \
            case opcode_##name: {               \
                value_t _right = pop();         \
                value_t _left = pop();          \
                push(v_##name(_left, _right));  \
                break;                          \
            }

        case_bin_op(add) case_bin_op(sub)
        case_bin_op(mul) case_bin_op(div) case_bin_op(mod)
        case_bin_op(eq) case_bin_op(neq)
        case_bin_op(gt) case_bin_op(lt)
        case_bin_op(gte) case_bin_op(lte)
        case_bin_op(in)

        default:
            die(v_to_string(v_add(v_string("unknown opcode "),
                                  v_number(opcode))));
        }
    }
}

static const char *opcode_names[opcode__count + 1] = {
#define X(name) #name,
# include "opcode.def"
#undef X
};

enum opcode string_to_opcode(const char *s) {
    for (int i = 0; i < opcode__count; i++) {
        if (opcode_names[i] && strcmp(opcode_names[i], s) == 0) {
            return i;
        }
    }
    return -1;
}

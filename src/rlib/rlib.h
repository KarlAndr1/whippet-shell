#ifndef RLIB_H_INCLUDED
#define RLIB_H_INCLUDED

/*
#define DEF_OP(name) struct r_val name##_callback(struct parse_node **args, unsigned n_args)

#define INCLUDE_OP(name, sym_str, arity) int_env_set(env, (lstring) { .str = sym_str, .len = sizeof(sym_str) - 1 }, int_register_extern_fn(name##_callback, arity), 1, 1) 
*/

extern const char *get_static_src();

#include "../interpreter/interpreter.h"

struct rlib_op {
	struct r_val fn_v;
	union {
		extern_callback_fn fn;
		extern_callback_runtime_fn runtime_fn;
	};
	int arity, type;
	char loaded;
	lstring sym_name;
};

#define DEF_OP(name, sym, arity_v) { .fn = name##_rlib_callback, .arity = arity_v, .loaded = 0, .sym_name = { sym, sizeof(sym) - 1 }, .type = 0 }
#define DEF_R_OP(name, sym, arity_v) { .runtime_fn = name##_rlib_callback_r, .arity = arity_v, .loaded = 0, .sym_name = { sym, sizeof(sym) - 1 }, .type = 1 }

#define DEF_ALIAS(sym) { .type = 2, .sym_name = { sym, sizeof(sym) - 1} }

#define DECL_OP(name) struct r_val name##_rlib_callback(struct parse_node **args, unsigned n_args, struct interp_env *env, const char *src_name, struct parse_node *expr)
#define DECL_R_OP(name) struct r_val name##_rlib_callback_r(struct r_val *args, unsigned n_args, struct interp_env *env, const char *src_name)

#define LOAD_RLIB(array) \
for(int i = 0; i < sizeof(array)/sizeof(array[0]); i++) { \
	S_ASSERT(!array[i].loaded); \
	array[i].loaded = 1; \
	if(array[i].type == 0) \
		array[i].fn_v = int_register_extern_fn(array[i].fn, array[i].arity); \
	else if(array[i].type == 1) \
		array[i].fn_v = int_register_extern_runtime_fn(array[i].runtime_fn, array[i].arity); \
	else if(array[i].type == 2) \
		array[i].fn_v = array[i - 1].fn_v; \
}

#define PUT_RLIB(array, env) for(int i = 0; i < sizeof(array)/sizeof(array[0]); i++) { S_ASSERT(array[i].loaded); int_env_set(env, array[i].sym_name, array[i].fn_v, 1, 1); }

int r_val_as_bool(struct r_val val);

int cmp_r_vals(struct r_val a, struct r_val b);
int r_vals_less(struct r_val a, struct r_val b);
int r_vals_greater(struct r_val a, struct r_val b);

char *r_string_to_cstr(const struct r_string *str);
struct r_string *cstr_to_rstring(const char *str);

#endif

#ifndef INTERPRETER_H_INCLUDED
#define INTERPRETER_H_INCLUDED

#include "../parser/parser.h"
#include "../proj_utils.h"
#include "../proj_defs.h"

enum {
	TYPE_NULL,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_STR,
	TYPE_FN,
	TYPE_EXT_FN,
	TYPE_ERR,
	TYPE_ARRAY
};

struct interp_env;
typedef unsigned long long extern_fn;

/*
struct r_intern_fn {
	int arity;
	lstring arg_names[];
	//struct parse_node **args, *expr;
}; */

struct r_array;

struct r_val {
	unsigned char type;
	union {
		//long long int_v;
		//double float_v;
		r_int int_v;
		r_float float_v;
		struct r_string *str_v;
		struct parse_node *fn;
		//struct r_intern_fn *fn;
		extern_fn ext_fn;
		struct r_array *array_v;
	};
};

struct r_array {
	unsigned len, ref_c;
	struct r_val items[];
};

void int_decr_refcount(struct r_val val);

void int_incr_refcount(struct r_val val);

typedef struct r_val (*extern_callback_fn)(struct parse_node **, unsigned, struct interp_env *, const char *, struct parse_node *);
typedef struct r_val (*extern_callback_runtime_fn)(struct r_val *, unsigned, struct interp_env *, const char *);

struct r_val int_register_extern_fn(extern_callback_fn fn, int arity);
struct r_val int_register_extern_runtime_fn(extern_callback_runtime_fn fn, int arity);
void int_clear_extern_fns();

struct interp_env *int_new_env();
void int_free_env(struct interp_env *env);

const struct r_val *int_env_get(struct interp_env *env, lstring name);
int int_env_set(struct interp_env *env, lstring name, struct r_val val, int is_new, int is_const);

struct r_val int_eval_expr(struct parse_node *fn, struct interp_env *env, const char *src_name);

struct r_val int_call_r_fn(struct r_val fn, struct r_val *args, unsigned n_args, struct interp_env *env, const char *src_name);

#include <stdio.h>

FILE *int_get_stdout(struct interp_env *env);
FILE *int_get_errout(struct interp_env *env);
FILE *int_get_stdin(struct interp_env *env);

FILE *int_set_stdout(struct interp_env *env, FILE *f);
FILE *int_set_stdin(struct interp_env *env, FILE *f);

void int_printerr(struct interp_env *env, const char *msg, struct r_val *args, unsigned n_args);

#endif

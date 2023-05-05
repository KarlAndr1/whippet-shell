#include "interpreter.h"

#include "interpreter_fmt.h"

#include "../proj_utils.h"
#include <string.h>

#include <stdlib.h>
#include <unistd.h>

#include "interpreter_config.h"

#include <errno.h>

struct extern_fn_container {
	union {
		extern_callback_fn fn;
		extern_callback_runtime_fn runtime_fn;
	};
	int arity, type;
};

struct { struct extern_fn_container *items; size_t len, cap; } external_functions;

static struct r_val register_extern_fn(struct extern_fn_container fn) {
	if(external_functions.len == external_functions.cap) {
		external_functions.cap += 1;
		external_functions.cap *= 2;
		external_functions.items = SREALLOC(struct extern_fn_container, external_functions.items, external_functions.cap);
	}
	
	external_functions.items[external_functions.len] = fn;
	
	return (struct r_val) { .type = TYPE_EXT_FN, .ext_fn = external_functions.len++ };
}

struct r_val int_register_extern_fn(extern_callback_fn fn, int arity) {
	return register_extern_fn( (struct extern_fn_container) { .fn = fn, .arity = arity, .type = 0 } );
}

struct r_val int_register_extern_runtime_fn(extern_callback_runtime_fn fn, int arity) {
	return register_extern_fn( (struct extern_fn_container) { .runtime_fn = fn, .arity = arity, .type = 1 } );
}

static struct extern_fn_container *get_extern_fn(extern_fn fn) {
	if(fn >= external_functions.len)
		return NULL;
	
	return &external_functions.items[fn];
}

void int_clear_extern_fns() {
	external_functions.cap = 0;
	external_functions.len = 0;
	s_dealloc(external_functions.items);
	external_functions.items = NULL;
}

void int_decr_refcount(struct r_val val) {
	switch(val.type) {
		case TYPE_STR:
			if(--val.str_v->ref_c == 0)
				s_dealloc(val.str_v);
			break;
		
		case TYPE_ARRAY:
			if(--val.array_v->ref_c == 0) {
				for(unsigned i = 0; i < val.array_v->len; i++) {
					int_decr_refcount(val.array_v->items[i]);
				}
				s_dealloc(val.array_v);
			}
			break;
	}
}

void int_incr_refcount(struct r_val val) {
	switch(val.type) {
		case TYPE_STR:
			val.str_v->ref_c++;
			break;
		
		case TYPE_ARRAY:
			val.array_v->ref_c++;
			break;
	}
}

enum {
	ENTRY_FLAG_NULL,
	ENTRY_FLAG_DEFAULT,
	ENTRY_FLAG_CONST
};

struct env_entry {
	lstring name;
	struct r_val val;
	unsigned char flag;
	
	struct env_entry *next;
};

struct interp_env {
	unsigned long long n_entries;
	struct env_entry *root;
	FILE *err_out, *std_out, *std_in;
};

struct interp_env *int_new_env() {
	struct interp_env *env = SALLOC(struct interp_env);
	env->n_entries = 0;
	env->root = NULL;
	
	env->std_out = stdout;
	env->err_out = stderr;
	env->std_in = stdin;

	return env;
}

void int_free_env(struct interp_env *env) {
	
	for(struct env_entry *entry = env->root, *next; entry != NULL; entry = next) {
		next = entry->next;
		int_decr_refcount(entry->val);
		s_dealloc(entry);
	}
	
	s_dealloc(env);
}

const struct r_val *int_env_get(struct interp_env *env, lstring name) {
	for(struct env_entry *entry = env->root; entry != NULL; entry = entry->next) {
		if(lstring_cmp(&entry->name, &name))
			return &entry->val;
	}
	return NULL;
}

int int_env_set(struct interp_env *env, lstring name, struct r_val val, int is_new, int is_const) {
	for(struct env_entry *entry = env->root; entry != NULL; entry = entry->next) {
		if(lstring_cmp(&entry->name, &name)) {
			if(entry->flag == ENTRY_FLAG_CONST)
				return -1;
			
			int_decr_refcount(entry->val);
			int_incr_refcount(val);
			entry->val = val;
			return 1;
		}
	}
	
	struct env_entry *new_var = SALLOC(struct env_entry);
	new_var->val = val;
	new_var->name = name; //NOTE THAT THE ENVIRONMENT IS NOT RESPONSIBLE FOR THE LIFETIME OF THE NAME STRING; i.e the string must be kept around for 
	//at least as long as the environment uses it.
	new_var->flag = is_const ? ENTRY_FLAG_CONST : ENTRY_FLAG_DEFAULT;
	new_var->next = env->root;
	env->root = new_var;
	
	int_incr_refcount(val);
	
	return 1;
}

static int match_arity(unsigned args, int arity) {
	if(arity < 0) {
		arity = -(arity + 1); // -1 means completely variadic, -2 means at least 1 argument, -3 at least 2 etc
		return args >= arity;
	}
	
	return args == arity;
}

#define R_VAL_NULL ((struct r_val) { .type = TYPE_NULL })

static const char *current_src_name;
static struct interp_env *current_env;

static struct r_val eval_expr(struct parse_node *expr);

#define ARG_BUFFER_SIZE 32

struct r_val int_call_r_fn(struct r_val fn, struct r_val *args, unsigned n_args, struct interp_env *env, const char *src_name) {
	if(fn.type == TYPE_FN) {
		if(n_args + 1 != fn.fn->expr.n_args)
			return R_VAL_NULL;
		
		for(unsigned i = 0; i < n_args; i++) {
			//struct r_val arg_v = eval_expr(args[i]);
			int res = int_env_set(env, fn.fn->expr.args[i]->str, args[i], 0, 0);
			if(!res) //If it's trying to override a constant as an argument
				return R_VAL_NULL;
		}
		
		return eval_expr(fn.fn->expr.args[n_args]);
	} else if(fn.type == TYPE_EXT_FN) {
		
		struct extern_fn_container *ext_fn = get_extern_fn(fn.ext_fn);
		if(ext_fn == NULL)
			return R_VAL_NULL;
		
		if(!match_arity(n_args, ext_fn->arity))
			return R_VAL_NULL;
		
		if(ext_fn->type == 0) {
			return R_VAL_NULL;
		} else if(ext_fn->type == 1) {
			if(n_args > ARG_BUFFER_SIZE) {
				fprintf(env->err_out, "Error: Argument count (%u) exceeds argument buffer size (%u).\n", n_args, (unsigned) ARG_BUFFER_SIZE);
				return R_VAL_NULL;
			}
			
			struct r_val arg_val_buffer[ARG_BUFFER_SIZE];
			
			for(unsigned i = 0; i < n_args; i++) {
				arg_val_buffer[i] = args[i];
				int_incr_refcount(arg_val_buffer[i]);
			}
			
			struct r_val res = ext_fn->runtime_fn(arg_val_buffer, n_args, env, src_name);
			
			
			for(unsigned i = 0; i < n_args; i++) {
				int_decr_refcount(arg_val_buffer[i]);
			}
			
			return res;
		}
	}
	
	return R_VAL_NULL;
}

struct r_val int_call_fn(struct r_val fn, struct parse_node **args, unsigned n_args, struct interp_env *env, const char *src_name, struct parse_node *expr) {
	if(fn.type == TYPE_EXT_FN) {
		struct extern_fn_container *ext_fn = get_extern_fn(fn.ext_fn);
		if(ext_fn == NULL)
			return R_VAL_NULL;
		
		if(!match_arity(n_args, ext_fn->arity))
			return R_VAL_NULL;
		
		if(ext_fn->type == 0) {
			struct r_val v = ext_fn->fn(args, n_args, env, src_name, expr);
			int_incr_refcount(v);
			return v;
		} else if(ext_fn->type == 1) {
			
			struct r_val arg_val_buffer[ARG_BUFFER_SIZE];
			
			if(n_args > ARG_BUFFER_SIZE) {
				fprintf(env->err_out, "Error: Argument count (%u) exceeds argument buffer size (%u).\n", n_args, (unsigned) ARG_BUFFER_SIZE);
				return R_VAL_NULL;
			}
			
			for(unsigned i = 0; i < n_args; i++) {
				arg_val_buffer[i] = eval_expr(args[i]);
			}
			
			struct r_val res = ext_fn->runtime_fn(arg_val_buffer, n_args, env, src_name);
			
			for(unsigned i = 0; i < n_args; i++) {
				int_decr_refcount(arg_val_buffer[i]);
			}
			
			return res;
		}
		
		S_ASSERT(false); //If the function isnt a type 0 or type 1
		return R_VAL_NULL;
		
	} else if(fn.type == TYPE_FN) {
		
		if(n_args + 1 != fn.fn->expr.n_args)
			return R_VAL_NULL;
		
		for(unsigned i = 0; i < n_args; i++) {
			struct r_val arg_v = eval_expr(args[i]);
			int res = int_env_set(env, fn.fn->expr.args[i]->str, arg_v, 0, 0);
			int_decr_refcount(arg_v);
			if(!res) //If it's trying to override a constant as an argument
				return R_VAL_NULL;
		}
		
		return eval_expr(fn.fn->expr.args[n_args]);
	} else {
		return R_VAL_NULL;
	}
}

#include <string.h>
#include <sys/wait.h>

#define ARG_STRING_BUFFER_SIZE 1024

static char **args_to_exec_commands(lstring com, struct r_val *args, unsigned n_args, memory_region *region) {
	char *arg_buff = nralloc(region, ARG_STRING_BUFFER_SIZE, char);
	char *arg_buff_end = arg_buff + ARG_STRING_BUFFER_SIZE;

	char *com_str = lstring_to_cstr(com, region);
	
	unsigned n_total_args = 0;
	for(unsigned i = 0; i < n_args; i++) {
		if(args[i].type == TYPE_ARRAY)
			n_total_args += args[i].array_v->len;
		else
			n_total_args++;
	}
	
	char **arg_strs = nralloc(region, n_total_args + 2, char*);
	arg_strs[n_total_args + 1] = NULL;
	arg_strs[0] = com_str;
	
	unsigned arg_str_i = 0;
	for(unsigned i = 0; i < n_args; i++) {
		struct r_val arg_v = args[i];
		if(arg_v.type == TYPE_ARRAY) { //TODO: CONTINUE THIS
			for(int j = 0; j < arg_v.array_v->len; j++) {
				char *arg_s = arg_buff;
				arg_buff = fmt_write_r_val_to_buff(arg_buff, arg_buff_end, arg_v.array_v->items[j], true);
				if(arg_buff == NULL)
					return NULL;
				arg_strs[1 + arg_str_i++] = arg_s;
			}
		} else {
			char *arg_s = arg_buff;
			arg_buff = fmt_write_r_val_to_buff(arg_buff, arg_buff_end, arg_v, true);
			if(arg_buff == NULL)
				return NULL;
			arg_strs[1 + arg_str_i++] = arg_s;
		}
	}
	
	return arg_strs;
}

static int exec_command(FILE *err_out, lstring com, struct r_val *args, unsigned n_args, struct interp_env *env) {
	
	memory_region *tmp_region = NEW_REGION();
	int exec_status = -1;
	
	char **arg_strs = args_to_exec_commands(com, args, n_args, tmp_region);
	
	if(arg_strs == NULL) {
		fprintf(err_out, "Unable to execute command: Arguments dont fit into string buffer (%u bytes)\n", (unsigned) ARG_STRING_BUFFER_SIZE);
		goto ERR;
	}
	
	const char *com_str = arg_strs[0];
	
	printf("COM (%s): ", com_str);
	for(char **c = arg_strs; *c != NULL; c++) {
		fputs(*c, stdout);
		putchar(' ');
	}
	putchar('\n');
	

	bool needs_user_approve = interpreter_get_config()->user_approve_commands;
	bool approved = true;
	
	if(needs_user_approve)
		approved = y_or_n_prompt(stdout, stdin, "Approve?");
	
	if(approved) {
		pid_t res = fork();
		switch(res) {
			
			case -1:
				exec_status = -1;
				fprintf(err_out, "Unable to fork: %s\n", strerror(errno));
				break;
			
			case 0:
				//Inside the newly created child process
				if(env->std_out != stdout) {
					dup2(fileno(env->std_out), STDOUT_FILENO);
				}
				if(env->std_in != stdin) {
					dup2(fileno(env->std_in), STDIN_FILENO);
				}
				execvp(com_str, arg_strs);
				
				//This only happens if it was unable to run the program
				fprintf(err_out, "Unable to exec '%s': %s\n", com_str, strerror(errno));
				
				exit(-1);
			
			default: {
				//Everything went well
				
				int status;
				WAIT:
				
				waitpid(res, &status, 0);
				if(WIFEXITED(status)) {
					exec_status = WEXITSTATUS(status);
					break;
				} else if(WIFSIGNALED(status)) {
					exec_status = -2;
					break;
				}
				goto WAIT;
			}
		}
	}
	
	free_memory_region(tmp_region);
	return exec_status;
	
	ERR:
	free_memory_region(tmp_region);
	
	return exec_status;
}

static struct r_val eval_expr(struct parse_node *expr) {
	switch(expr->type) {
		
		case PNODE_EXPR: { 
			if(expr->expr.op->type == PNODE_SYM) {
				const struct r_val *var = int_env_get(current_env, expr->expr.op->str);
				if(var == NULL) {
					struct r_val *args = NSALLOC(struct r_val, expr->expr.n_args);
					unsigned n_args = expr->expr.n_args;
					
					for(unsigned i = 0; i < n_args; i++) {
						args[i] = eval_expr(expr->expr.args[i]);
					}
					exec_command(current_env->err_out, expr->expr.op->str, args, n_args, current_env);
					
					for(unsigned i = 0; i < n_args; i++) {
						int_decr_refcount(args[i]);
					}
					
					s_dealloc(args);
					
					return R_VAL_NULL;
				}
				return int_call_fn(*var, expr->expr.args, expr->expr.n_args, current_env, current_src_name, expr);
			} else {
				return int_call_fn(eval_expr(expr->expr.op), expr->expr.args, expr->expr.n_args, current_env, current_src_name, expr);
			}
		} break;
		
		case PNODE_BLOCK: {
			struct r_val res = { .type = TYPE_NULL };
			for(unsigned i = 0; i < expr->expr.n_args; i++) {
				int_decr_refcount(res);
				res = eval_expr(expr->expr.args[i]);
			}
			return res;
		}
		
		case PNODE_INT:
			return (struct r_val) { .type = TYPE_INT, .int_v = expr->int_v };
		
		case PNODE_VAR: {
			const struct r_val *var = int_env_get(current_env, expr->str);
			if(var == NULL)
				return R_VAL_NULL;
			int_incr_refcount(*var);
			return *var;
		}
		
		case PNODE_SYM: {
			lstring sym_str = expr->str;
			struct r_string *new_str = s_alloc(sizeof(struct r_string) + sym_str.len);
			memcpy((char *) new_str->str, sym_str.str, sym_str.len);
			new_str->ref_c = 1;
			//new_str->ref_c = 0;
			new_str->len = sym_str.len;
			
			return (struct r_val) { .type = TYPE_STR, .str_v = new_str }; //This isn't ideal; the interpreter will allocate a new string every time a string constant
			//is used, even if it's the same one multiple times in a loop.
		}
		
		default:
			S_ASSERT(false);
			return (struct r_val) { .type = TYPE_NULL };
	}
}

struct r_val int_eval_expr(struct parse_node *fn, struct interp_env *env, const char *src_name) {
	const char *tmp_name = current_src_name;
	struct interp_env *tmp_env = current_env;
	
	current_src_name = src_name;
	current_env = env;
	
	struct r_val res = eval_expr(fn);
	
	current_src_name = tmp_name;
	current_env = tmp_env;
	
	return res;
}

FILE *int_get_stdout(struct interp_env *env) {
	return env->std_out;
}

FILE *int_get_errout(struct interp_env *env) {
	return env->err_out;
}

FILE *int_get_stdin(struct interp_env *env) {
	return env->std_in;
}

FILE *int_set_stdout(struct interp_env *env, FILE *f) {
	FILE *old_stdout = env->std_out;
	env->std_out = f;
	return old_stdout;
}

FILE *int_set_stdin(struct interp_env *env, FILE *f) {
	FILE *old_stdin = env->std_in;
	env->std_in = f;
	return old_stdin;
}

void int_printerr(struct interp_env *env, const char *msg, struct r_val *args, unsigned n_args) {
	FILE *err_out = env->err_out;
	
	const char *str_start = msg;
	const char *c = msg;
	for(; *c != '\0'; c++) {
		if(*c == '%') {
			
			if(str_start != c)
				print_len_str(err_out, str_start, c - str_start);
			
			c++;
			int n = *c - '0';
			S_ASSERT(n >= 0 && n < n_args);
			S_ASSERT(*c != '\0');
			
			fmt_print_r_val(err_out, args[n]);
			
			str_start = c + 1;
			
			continue;
		}
	}
	
	if(str_start != c) {
		print_len_str(err_out, str_start, c - str_start);
	}
	
	putc('\n', err_out);
}

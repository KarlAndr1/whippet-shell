#include "rlib_basic.h"

#include "rlib.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../interpreter/interpreter_config.h"
#include "../interpreter/interpreter_utils.h"

#include "../parser/parser_fmt.h"

#include <stdlib.h>

DECL_OP(let) {
	S_ASSERT(n_args == 2);
	
	if(args[0]->type != PNODE_SYM) {
		fmt_blame_parse_node(int_get_errout(env), "Expected symbol as variable name, got %.", args[0], get_static_src(), src_name);
		//INT_PRINTERR(env, "Expected symbol as variable name, got %0", args[0]);
		return (struct r_val) { .type = TYPE_NULL };
	}
	
	lstring sym_name = args[0]->str;
	
	struct r_val assign_val = int_eval_expr(args[1], env, src_name);
	
	int_env_set(env, sym_name, assign_val, 1, 0);
	int_decr_refcount(assign_val);
	
	return assign_val;
}

DECL_OP(add) {
	r_int int_sum = 0;
	
	for(unsigned i = 0; i < n_args; i++) {
		struct r_val arg_v = int_eval_expr(args[i], env, src_name);
		
		if(arg_v.type == TYPE_INT)
			int_sum += arg_v.int_v;
		else {
			int_decr_refcount(arg_v);
			return (struct r_val) { .type = TYPE_NULL };
		}
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = int_sum };
}

#include "../interpreter/interpreter_fmt.h"

DECL_OP(print) {

	FILE *to_file = int_get_stdout(env);
	
	for(unsigned i = 0; i < n_args; i++) {
		
		struct r_val val = int_eval_expr(args[i], env, src_name);
		fmt_print_r_val(to_file, val);
		int_decr_refcount(val);
		
		if(i != n_args - 1)
			putchar(' ');
	}
	
	putc('\n', to_file);
	
	return (struct r_val) { .type = TYPE_NULL };
}


DECL_OP(lambda) {
	struct r_val fn_val = { .type = TYPE_FN, .fn = expr };
	
	for(unsigned i = 0; i < n_args - 1; i++) {
		if(args[i]->type != PNODE_SYM) {
			fmt_blame_parse_node(int_get_errout(env), "Invalid function argument name: '%'", args[i], get_static_src(), src_name);
			return (struct r_val) { .type = TYPE_NULL };
		}
	}
	
	return fn_val;
}

DECL_OP(sub) {
	r_int int_diff;
	
	struct r_val first = int_eval_expr(args[0], env, src_name);
	int_decr_refcount(first);
	
	if(first.type != TYPE_INT)
		return (struct r_val) { .type = TYPE_NULL };
	
	int_diff = first.int_v;
	
	if(n_args == 1)
		return (struct r_val) { .type = TYPE_INT, .int_v = -int_diff };
	
	for(unsigned i = 1; i < n_args; i++) {
		struct r_val val = int_eval_expr(args[i], env, src_name);
		
		if(val.type == TYPE_INT) {
			int_diff -= val.int_v;
		} else {
			int_decr_refcount(val);
			return (struct r_val) { .type = TYPE_NULL };
		}
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = int_diff };
}

DECL_OP(mul) {
	r_int int_prod = 1;
	
	for(unsigned i = 0; i < n_args; i++) {
		struct r_val val = int_eval_expr(args[i], env, src_name);

		if(val.type == TYPE_INT)
			int_prod *= val.int_v;
		else {
			int_decr_refcount(val);
			return (struct r_val) { .type = TYPE_NULL };
		}
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = int_prod };
}


DECL_OP(div) {
	r_int int_res;
	
	struct r_val first = int_eval_expr(args[0], env, src_name);
	int_decr_refcount(first);
	
	if(first.type != TYPE_INT)
		return (struct r_val) { .type = TYPE_NULL };
	
	int_res = first.int_v;
	
	if(n_args == 1)
		return (struct r_val) { .type = TYPE_INT, .int_v = int_res };
	
	for(unsigned i = 1; i < n_args; i++) {
		struct r_val val = int_eval_expr(args[i], env, src_name);
		
		if(val.type == TYPE_INT) {
			int_res /= val.int_v;
		} else {
			int_decr_refcount(val);
			return (struct r_val) { .type = TYPE_NULL };
		}
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = int_res };
}

DECL_OP(printf) {
	struct r_val fstr = int_eval_expr(args[0], env, src_name);
	
	struct r_val *arg_v = NSALLOC(struct r_val, n_args - 1);
	
	for(unsigned i = 0; i < n_args - 1; i++) {
		arg_v[i] = int_eval_expr(args[i + 1], env, src_name);
	}
	
	if(fstr.type != TYPE_STR)
		goto EXIT;
	
	FILE *to_file = int_get_stdout(env);
	
	const char *buff = fstr.str_v->str;
	for(const char *c = fstr.str_v->str, *end = fstr.str_v->str + fstr.str_v->len; c < end; c++) {
		if(*c == '%') {
			if(buff != c) {
				print_len_str(to_file, buff, c - buff);
			}
			c++;
			if(c == end)
				break;
			int arg = *c - '0';
			if(arg < n_args - 1 && arg >= 0) {
				fmt_print_r_val(to_file, arg_v[arg]);
			} else if(*c == 'n') {
				putchar('\n');
			}
			
			buff = c + 1;
		}
	}
	
	if(buff < fstr.str_v->str + fstr.str_v->len) {
		print_len_str(to_file, buff, fstr.str_v->len - (buff - fstr.str_v->str));
	}
	
	EXIT:
	
	for(unsigned i = 0; i < n_args - 1; i++)
		int_decr_refcount(arg_v[i]);
	
	s_dealloc(arg_v);
	
	int_decr_refcount(fstr);
	return (struct r_val) { .type = TYPE_NULL };
}

DECL_R_OP(cd) {
	//struct r_val path_v = int_eval_expr(args[0], env, src_name);

	char path[512];
	if(fmt_write_r_val_to_buff(path, path + 512, args[0], true) != NULL) {
		if(chdir(path)) {
			fprintf(int_get_errout(env), "Unable to change working directory to '%s': %s\n", path, strerror(errno));
		}
	}
	//int_decr_refcount(path_v);
	
	return (struct r_val) { .type = TYPE_NULL };
}

DECL_OP(if) {
	struct r_val cond = int_eval_expr(args[0], env, src_name);
	int cond_b = r_val_as_bool(cond);
	int_decr_refcount(cond);
	
	if(cond_b) {
		return int_eval_expr(args[1], env, src_name);
	} else {
		if(n_args < 3)
			return (struct r_val) { .type = TYPE_NULL };
		
		return int_eval_expr(args[2], env, src_name);
	}
}

DECL_R_OP(eq) {
	for(unsigned i = 1; i < n_args; i++) {
		if(!cmp_r_vals(args[0], args[i]))
			return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = 1 };
}

DECL_R_OP(less) {
	for(unsigned i = 1; i < n_args; i++) {
		if(!r_vals_less(args[i-1], args[i]))
			return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = 1 };
}

DECL_R_OP(greater) {
	for(unsigned i = 1; i < n_args; i++) {
		if(!r_vals_greater(args[i-1], args[i]))
			return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = 1 };
}

DECL_OP(do) {
	struct r_val val = (struct r_val) { .type = TYPE_NULL };
	for(unsigned i = 0; i < n_args; i++) {
		int_decr_refcount(val);
		val = int_eval_expr(args[i], env, src_name);
	}
	
	return val;
}

DECL_R_OP(array) {
	struct r_array *array = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * n_args);
	array->len = n_args;
	array->ref_c = 1;
	
	for(unsigned i = 0; i < n_args; i++) {
		int_incr_refcount(args[i]);
		array->items[i] = args[i];
	}
	
	return (struct r_val) { .type = TYPE_ARRAY, .array_v = array };
}

DECL_R_OP(map) { //NOTE TO SELF: Remember that the args array might not remain intact if another function is called via int_call_r_fn or such.
	if(args[0].type != TYPE_ARRAY)
		return (struct r_val) { .type = TYPE_NULL };
	
	struct r_array *src_array = args[0].array_v;
	struct r_val fn = args[1];
	
	struct r_array *array = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * src_array->len);
	array->len = src_array->len;
	array->ref_c = 1;
	
	for(unsigned i = 0; i < array->len; i++) {
		struct r_val fn_arg = src_array->items[i];
		struct r_val res = int_call_r_fn(fn, &fn_arg, 1, env, src_name);
		array->items[i] = res;
	}
	
	return (struct r_val) { .type = TYPE_ARRAY, .array_v = array };
}

DECL_R_OP(filter) {
	if(args[0].type != TYPE_ARRAY)
		return (struct r_val) { .type = TYPE_NULL };
	
	struct r_array *src_array = args[0].array_v;
	struct r_val *buffer = s_alloc(sizeof(struct r_val) * src_array->len);
	unsigned n_out = 0;
	
	struct r_val fn = args[1];
	
	for(int i = 0; i < src_array->len; i++) {
		struct r_val fn_arg = src_array->items[i];
		struct r_val res = int_call_r_fn(fn, &fn_arg, 1, env, src_name);
		if(r_val_as_bool(res)) {
			buffer[n_out++] = fn_arg;
			int_incr_refcount(fn_arg);
		}
		int_decr_refcount(res);
	}
	
	struct r_array *out_array = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * n_out);
	out_array->len = n_out;
	out_array->ref_c = 1;
	
	memcpy(out_array->items, buffer, sizeof(struct r_val) * n_out);
	
	s_dealloc(buffer);
	
	return (struct r_val) { .type = TYPE_ARRAY, .array_v = out_array };
}

DECL_OP(open) {
	struct r_val path = int_eval_expr(args[0], env, src_name);
	struct r_val mode = int_eval_expr(args[1], env, src_name);
	
	if(path.type != TYPE_STR || mode.type != TYPE_STR)
		goto ERR;
	
	struct r_string *mode_str = mode.str_v;
	enum { M_READ, M_WRITE } open_mode;
	const char *mode_cstr;
	
	if(mode_str->len == 1 && mode_str->str[0] == 'r') {
		open_mode = M_READ;
		mode_cstr = "r";
	} else if(mode_str->len == 1 && mode_str->str[0] == 'w') {
		open_mode = M_WRITE;
		mode_cstr = "w";
	} else {
		goto ERR;
	}
	
	char *c_path = s_alloc(path.str_v->len + 1);
	
	memcpy(c_path, path.str_v->str, path.str_v->len);
	c_path[path.str_v->len] = '\0';
	
	int_decr_refcount(path);
	int_decr_refcount(mode);
	
	FILE *f;
	
	bool manual_approve = interpreter_get_config()->user_approve_commands;
	int err_n = -1;
	
	if(manual_approve) {
		printf("Attempting to open %s (mode %s), ", c_path, mode_cstr);
		if(y_or_n_prompt(stdout, stdin, NULL)) {
			f = fopen(c_path, mode_cstr);
			err_n = errno;
		} else {
			f = NULL;
		}
	} else {
		f = fopen(c_path, mode_cstr);
		err_n = errno;
	}
	
	if(f == NULL) {
		FILE *err = int_get_errout(env);
		fprintf(err, "Unable to open file '%s' (%s): ", c_path, mode_cstr);
		if(errno != -1) {
			fputs(strerror(err_n), err);
		} else {
			fputs("Manually denied", err);
		}
		putc('\n', err);

		s_dealloc(c_path);
		
		return (struct r_val) { .type = TYPE_NULL };
	}
	
	s_dealloc(c_path);
	
	FILE *restore_in = NULL, *restore_out = NULL;
	switch(open_mode) {
		case M_READ:
			restore_in = int_set_stdin(env, f);
		break;
		
		case M_WRITE:
			restore_out = int_set_stdout(env, f);
		break;
	}
	//FILE *restore = int_set_stdout(env, f);
	
	struct r_val res = int_eval_expr(args[2], env, src_name);
	int_decr_refcount(res);
	
	if(restore_in != NULL)
		int_set_stdin(env, restore_in);
	if(restore_out != NULL)
		int_set_stdout(env, restore_out);
	//int_set_stdout(env, restore);
	
	fclose(f);
	
	return (struct r_val) { .type = TYPE_NULL };
	
	ERR:
	int_decr_refcount(path);
	int_decr_refcount(mode);
	return (struct r_val) { .type = TYPE_NULL };
}

DECL_R_OP(readline) {
	FILE *from_file = int_get_stdin(env);
	
	if(args[0].type != TYPE_NULL) {
		fmt_print_r_val(int_get_stdout(env), args[0]);
	}
	//int_decr_refcount(args[0]);
	
	char *buff, *end, *top;
	buff = s_alloc(8);
	top = buff;
	end = buff + 8;
	
	int res;
	while( (res = fgetc(from_file)) != EOF) {
		if(res == '\n')
			break;
		
		if(top == end) {
			size_t cap = end - buff;
			buff = s_realloc(buff, cap * 2);
			end = buff + cap * 2;
			top = buff + cap;
		}
		*(top++) = res;
	}
	if(res == EOF && top == buff) { //If nothing was read from the file/stream and an end of file was returned
		s_dealloc(buff);
		return (struct r_val) { .type = TYPE_NULL };
	}
	
	unsigned len = top - buff;
	struct r_string *str = s_alloc(sizeof(struct r_string) + len);
	memcpy((char *) str->str, buff, len);
	s_dealloc(buff);
	
	str->len = len;
	str->ref_c = 1;
	
	return (struct r_val) { .type = TYPE_STR, .str_v = str };
}

#include <ftw.h>

static struct { struct r_string **paths; size_t len, cap; } file_paths;

static int record_dir_file_entry(const char *path, const struct stat *sb, int typeflag) {
	if(typeflag != FTW_F)
		return 0;
	
	unsigned str_len = strlen(path);
	struct r_string *path_str = s_alloc(sizeof(struct r_string) + str_len);
	path_str->len = str_len;
	path_str->ref_c = 1;
	memcpy( (char *) path_str->str, path, str_len);
	
	if(file_paths.len == file_paths.cap) {
		file_paths.cap *= 2;
		file_paths.paths = SREALLOC(struct r_string *, file_paths.paths, file_paths.cap);
	}
	
	file_paths.paths[file_paths.len++] = path_str;
	
	return 0;
}

DECL_R_OP(indir) {
	if(args[0].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };	
	
	char *dir_path = s_alloc(args[0].str_v->len + 1);
	memcpy(dir_path, args[0].str_v->str, args[0].str_v->len);
	dir_path[args[0].str_v->len] = '\0';
	
	file_paths.len = 0;
	file_paths.cap = 8;
	file_paths.paths = s_alloc(sizeof(struct r_string *) * 8);
	
	if(ftw(dir_path, record_dir_file_entry, 8) == -1)
		goto ERR;
	
	struct r_array *array = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * file_paths.len);
	array->len = file_paths.len;
	array->ref_c = 1;
	for(unsigned i = 0; i < array->len; i++) {
		array->items[i] = (struct r_val) { .type = TYPE_STR, .str_v = file_paths.paths[i] };
	}
	
	s_dealloc(file_paths.paths);
	s_dealloc(dir_path);
	
	return (struct r_val) { .type = TYPE_ARRAY, .array_v = array };
	
	ERR:
	
	s_dealloc(file_paths.paths);
	s_dealloc(dir_path);
	
	return (struct r_val) { .type = TYPE_NULL };
}

DECL_R_OP(getenv) {
	if(args[0].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };
	
	char *var_name = s_alloc(args[0].str_v->len + 1); //Create a c string from the 0th argument
	var_name[args[0].str_v->len] = '\0';
	memcpy(var_name, args[0].str_v->str, args[0].str_v->len);
	
	char *val = getenv(var_name);
	
	s_dealloc(var_name);
	
	if(val == NULL)
		return (struct r_val) { .type = TYPE_NULL };
	
	unsigned val_len = strlen(val);
	struct r_string *str_res = s_alloc(sizeof(struct r_string) + val_len);
	str_res->len = val_len;
	memcpy( (char *) str_res->str, val, val_len);
	str_res->ref_c = 1;
	
	return (struct r_val) { .type = TYPE_STR, .str_v = str_res };
}

char tmp_charbuff[1024];

DECL_R_OP(setenv) {
	if(args[0].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };
	
	char *var_name = r_string_to_cstr(args[0].str_v);
	
	char *res = fmt_write_r_val_to_buff(tmp_charbuff, tmp_charbuff + sizeof(tmp_charbuff), args[1], true);
	if(res == NULL) { //I.e if the value couldn't fit in the buffer
		s_dealloc(var_name);
		return (struct r_val) { .type = TYPE_NULL };
	}
	
	setenv(var_name, tmp_charbuff, 1);
	
	s_dealloc(var_name);
	
	int_incr_refcount(args[1]);
	return args[1];
}

DECL_R_OP(index) {
	if(args[0].type != TYPE_ARRAY || args[1].type != TYPE_INT)
		return (struct r_val) { .type = TYPE_NULL };
	
	struct r_array *a = args[0].array_v;
	r_int i = args[1].int_v;
	
	if(i >= a->len)
		return (struct r_val) { .type = TYPE_NULL };
	
	if(i < 0) {
		i = (a->len + i) % a->len; //I.e index -1 is the same as len - 1
	}
	S_ASSERT(i >= 0 && i < a->len);
	return a->items[i];
}

static struct rlib_op ops[] = { //TODO: Change +, print, -, *, / and printf to be runtime functions
	DEF_OP(let, "let", 2),
	DEF_OP(add, "+", -1),
	DEF_OP(print, "print", -1),
	
	DEF_OP(lambda, "lambda", -2),
	DEF_ALIAS("^"),
	DEF_ALIAS("\\"),
	DEF_ALIAS("!"),
	DEF_ALIAS("λ"),
	
	DEF_OP(sub, "-", -2),
	DEF_OP(mul, "*", -1),
	DEF_OP(div, "/", -2),
	
	DEF_R_OP(cd, "cd", 1),
	
	DEF_OP(printf, "printf", -2),
	
	DEF_OP(if, "if", -3),
	
	DEF_R_OP(eq, "=", -3),
	DEF_R_OP(less, "<", -3),
	DEF_R_OP(greater, ">", -3),
	
	DEF_OP(do, "do", -1),
	
	DEF_R_OP(array, "array", -1),
	DEF_R_OP(map, "map", 2),
	DEF_R_OP(filter, "filter", 2),
	
	DEF_OP(open, "open", 3),
	
	DEF_R_OP(readline, "readline", 1),
	
	DEF_R_OP(indir, "indir", 1),
	
	DEF_R_OP(getenv, "getenv", 1),
	DEF_R_OP(setenv, "setenv", 2),
	
	DEF_R_OP(index, "index", 2)
};

static char loaded = 0;

void rlib_basic_load() {
	if(loaded)
		return;
	
	loaded = 1;
	LOAD_RLIB(ops);
}

void rlib_basic_put(struct interp_env *env) {
	PUT_RLIB(ops, env);
}

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "proj_utils.h"
#include "proj_config.h"

#include "parser/lexer.h"
#include "parser/parser_fmt.h"

#include "interpreter/interpreter.h"
#include "interpreter/interpreter_fmt.h"

#include "rlib/rlib_basic.h"
#include "rlib/rlib_strutils.h"
#include "rlib/rlib_extra.h"

#include "colour_defs.h"

#include <string.h>

#include "tui/tui_system.h"

#include "tui/highlighting.h"

#include "testing/tests.h"

static void main_err(int c, const char *msg) {
	fprintf(stderr, "Error (%i): %s\n", c, msg);
	exit(-1);
}

static void *main_alloc(size_t n) {
	void *mem = malloc(n);
	if(mem == NULL)
		main_err(ERR_OUT_OF_MEM, "Unable to alloc (out of memory?)");
	
	return mem;
}

static void *main_realloc(void *ptr, size_t n) {
	void *new_mem = realloc(ptr, n);
	if(new_mem == NULL)
		main_err(ERR_OUT_OF_MEM, "Unable to realloc (out of memory?)");
	
	return new_mem;
}	

static void print_prompt_msg(FILE *f) {
	#ifdef DEBUG
		fputs(COLOUR_THEME1 PROJ_NAME COLOUR_RESET ", " COLOUR_THEME2 VERSION_NAME " " VERSION_CODE " (" __DATE__ " - " __TIME__ ")" COLOUR_RESET "\n", f);
	#else
		fputs(COLOUR_THEME1 PROJ_NAME COLOUR_RESET ", " COLOUR_THEME2 VERSION_NAME " " VERSION_CODE COLOUR_RESET "\n", f);
	#endif
}

static size_t read_line(FILE *f, char *to_buff, size_t buff_len, bool *eof) {
	size_t chars_read = 0;
	
	*eof = false;
	
	int c;
	while( (c = getc(f)) != '\n' ) {
		if(c == EOF) {
			if(chars_read == 0) {
				*eof = true;
			} else {
				putchar('\n');
			}
			return chars_read;
		}
		if(chars_read < buff_len) {
			*(to_buff++) = c;
			chars_read++;
		}
	}
	
	return chars_read;
}

#include <signal.h>
//https://en.wikipedia.org/wiki/C_signal_handling

static void print_prompt_line(FILE *f) {
	#define PWD_BUFF_LEN 512
	char pwd_buff[PWD_BUFF_LEN];
	
	fputs(COLOUR_INFO, f);
	if(getcwd(pwd_buff, PWD_BUFF_LEN))
		fputs(pwd_buff, f);

	fputs(">" COLOUR_THEME1 " $ "PROJ_NAME ": ", f);
	fputs(COLOUR_RESET, f);
}

static void handle_signals(int signal_n) {
	putchar('\n');
	print_prompt_line(stdout);
	fflush(stdout);
	return;
}

static void put_libs(struct interp_env *env) {
	rlib_basic_put(env);
	rlib_strutils_put(env);
	rlib_extra_put(env);
}

static void run_prompt() {
	
	signal(SIGINT, handle_signals);
	
	print_prompt_msg(stdout);
	
	memory_region *symbol_region = NEW_REGION();
	memory_region *parse_region = NEW_REGION();
	
	struct interp_env *env = int_new_env();
	put_libs(env);
	
	#define LINE_BUFF_LEN 1024
	char line_buff[LINE_BUFF_LEN];
	
	while(true) {
		print_prompt_line(stdout);
		
		bool eof;
		size_t n_read = read_line(stdin, line_buff, LINE_BUFF_LEN - 1, &eof);
		S_ASSERT(n_read < LINE_BUFF_LEN);
		line_buff[n_read] = '\0';
		
		if(eof) {
			putchar('\n');
			break;
			//goto END;
		}
		
		if(cmp_len_strs(line_buff, n_read, "quit", 4) || (n_read == 1 && line_buff[0] == 'q'))
			break;
		
		struct parse_node *expr = par_parse("stdin", line_buff, parse_region, symbol_region);
		
		if(expr != NULL) {
			struct r_val res = int_eval_expr(expr, env, "stdin");
			if(res.type != TYPE_NULL) {
				fputs(COLOUR_THEME2, stdout);
				fmt_print_r_val(stdout, res);
				fputs(COLOUR_RESET, stdout);
				putchar('\n');
			}
			int_decr_refcount(res);
		}
	}
	
	//END:
	
	int_free_env(env);
	
	free_memory_region(parse_region);
	free_memory_region(symbol_region);
}

static void tui_drawline(const char *prompt_msg, const char *line, unsigned line_len, unsigned cursor_pos) {
	printf(COLOUR_INFO "%s>" COLOUR_THEME1 " $ " PROJ_NAME ": " COLOUR_RESET, prompt_msg);
	
	const char *match_bracket = NULL;
	const char *cursor_char = line + cursor_pos - 1;
	
	if(cursor_pos > 0 && (line[cursor_pos-1] == ')' || line[cursor_pos-1] == ']')) {
		unsigned bracket_counter = 0;
		for(int i = cursor_pos - 2; i >= 0; i--) {
			if(line[i] == ')' || line[i] == ']')
				bracket_counter++;
			
			if(line[i] == '(' || line[i] == '[') {
				if(bracket_counter == 0) {
					match_bracket = line + i;
					break;
				}
				bracket_counter--;
			}
		}
	}
	
	/*
	for(unsigned i = 0; i < line_len; i++) {
		//printf("%i,", (int) line[i]);
		putchar(line[i]);
	}
	*/
	
	for(const char *c = line; c < line + line_len; c++) {
		if(c == match_bracket) {
			fputs(COLOUR_THEME1, stdout);
			putchar(*c);
			fputs(COLOUR_RESET, stdout);
			continue;
		}
		if(c == cursor_char && match_bracket != NULL) {
			fputs(COLOUR_THEME1, stdout);
			putchar(*c);
			fputs(COLOUR_RESET, stdout);
			continue;
		}
		
		putchar(*c);
	}
}

static void draw_line_wrapper(const char *prompt_msg, const char *line, unsigned len, unsigned cursor_pos) {
	printf(COLOUR_INFO "%s>" COLOUR_THEME1 " $ " COLOUR_RESET, prompt_msg);
	highlight_draw_line(line, len, cursor_pos);
}

static void run_tui_prompt() {
	memory_region *region = NEW_REGION();
	
	print_prompt_msg(stdout);
	
	struct interp_env *env = int_new_env();
	put_libs(env);
	
	struct tui_state *input = new_tui_state(16, draw_line_wrapper);
	
	char pwd_buff[PWD_BUFF_LEN];
	if(!getcwd(pwd_buff, PWD_BUFF_LEN))
		pwd_buff[0] = '\0';
	
	//const char *in_s = 
	while(tui_readline(input, pwd_buff) == 0) {
		const char *line = tui_state_get_contents(input);
		if(strcmp(line, "quit") == 0)
			break;
		
		tui_deinit();
		
		struct parse_node *expr = par_parse("stdin", line, region, region);
		if(expr != NULL) {
			struct r_val res = int_eval_expr(expr, env, "stdin");
			if(res.type != TYPE_NULL) {
				fputs(COLOUR_THEME2, stdout);
				fmt_print_r_val(stdout, res);
				fputs(COLOUR_RESET "\n", stdout);
			}
			int_decr_refcount(res);
		}
		
		if(!getcwd(pwd_buff, PWD_BUFF_LEN))
			pwd_buff[0] = '\0';
		
		int err = tui_init();
		if(err != 0) {
			fprintf(stderr, "TUI initialisation error (%i)!\n", err);
			break;
		}
	}
	
	free_tui_state(input);
	
	int_free_env(env);
	free_memory_region(region);
}

const char *static_src = NULL;

const char *get_static_src() { //This is so that scripts can report errors with context, i.e showing parts of the source code 
	return static_src;
}

static int run_script(struct interp_env *opt_env, memory_region *r, const char *path, char **argv, int argc) {
	FILE *f = fopen(path, "r");
	if(f == NULL)
		return -1;
	
	int res_i = 0;
	
	char *src = read_alloc_file(f, true);
	const char *prev_static_src = src;
	static_src = src;
	
	fclose(f);
	
	bool tmp_env = opt_env == NULL;
	if(tmp_env) {
		S_ASSERT(r == NULL);
		
		opt_env = int_new_env();
		put_libs(opt_env);
		r = NEW_REGION();
	}
	
	S_ASSERT(argc > 0);
	struct r_array *arg_array = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * argc);
	arg_array->len = argc;
	arg_array->ref_c = 0; //The reference count is incremented to 1 when its set to a variable in the env struct
	for(int i = 0; i < argc; i++) {
		unsigned arg_len = strlen(argv[i]);
		struct r_string *arg = s_alloc(sizeof(struct r_string) + arg_len);
		memcpy((char *) arg->str, argv[i], arg_len);
		arg->len = arg_len;
		arg->ref_c = 1;
		arg_array->items[i] = (struct r_val) { .type = TYPE_STR, .str_v = arg };
	}
	int_env_set(opt_env, LSTRING("argv"), (struct r_val) { .type = TYPE_ARRAY, .array_v = arg_array }, 1, 1);
	
	struct parse_node *expr = par_parse(path, src, r, r);
	if(expr == NULL) {
		res_i = -2;
		goto END;
	}
	
	int_eval_expr(expr, opt_env, path);
	
	END:
	static_src = prev_static_src;
	s_dealloc(src);
	
	if(tmp_env) {
		int_free_env(opt_env);
		free_memory_region(r);
	}
	
	return res_i;
}

static void load_libs() {
	rlib_basic_load();
	rlib_strutils_load();
	rlib_extra_load();
}

#include "interpreter/interpreter_config.h"

static void print_version_and_exit() {
	print_prompt_msg(stdout);
	exit(0);
}

static int rich_terminal = SETTING_RICH_TERMINAL;

static int handle_arguments(int argc, char **argv) {
	int src_file_arg = -1;
	
	struct int_config interp_conf = {
		.user_approve_commands = SETTING_APPROVE_COMMANDS
	};
	
	for(int i = 1; i < argc; i++) {
	
		if(argv[i][0] != '-') {
			src_file_arg = i;
			break; //Arguments after the source file are ignored by the argument handler, since those are arguments that are passed to the executed script
		} else if(strcmp(argv[i], "-V") == 0)
			print_version_and_exit();
		else if(strcmp(argv[i], "--version") == 0)
			print_version_and_exit();
		else if(strcmp(argv[i], "--manual-approve") == 0)
			interp_conf.user_approve_commands = 1;
		else if(strcmp(argv[i], "--no-manual-approve") == 0)
			interp_conf.user_approve_commands = 0;
		else if(strcmp(argv[i], "--terminal-rich") == 0)
			rich_terminal = 1;
		else if(strcmp(argv[i], "--terminal-basic") == 0)
			rich_terminal = 0;
		else {
			printf(COLOUR_ERROR PROJ_NAME ": unkown option -- '%s'\n" COLOUR_RESET, argv[i]);
			exit(-1);
		}
	}
	
	interpreter_set_config(&interp_conf);
	
	return src_file_arg;
}

int main(int argc, char **argv) {
	proj_utils_init(main_err, main_alloc, main_realloc, free);

	int src_file_arg = handle_arguments(argc, argv);

	load_libs();
	
	DO_TESTS();
	
	int status = 0;

	if(src_file_arg == -1) {
		if(rich_terminal && tui_init() == 0) {
			run_tui_prompt();
			tui_deinit();
		} else {
			run_prompt();
		}
	} else {
		status = run_script(NULL, NULL, argv[src_file_arg], argv + src_file_arg, argc - src_file_arg);
	}
	
	int_clear_extern_fns();

	return status;
}

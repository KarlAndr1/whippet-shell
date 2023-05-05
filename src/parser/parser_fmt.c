#include "parser_fmt.h"

#include <stdio.h>

#include "../proj_utils.h"

static void print_tok(FILE *f, struct lex_token tok) {
	switch(tok.type) {
		
		case TOK_SYMBOL:
			print_len_str(f, tok.str.str, tok.str.len);
			fputs(" (Symbol)", f);
			//fprintf(f, "%s (Symbol)", tok.str.str
			//fprintf(f, "Symbol (%llu)", (unsigned long long) tok.sym);
			break;
		case TOK_INT_LITERAL:
			fprintf(f, "%lli (Integer)", (long long) tok.int_literal);
			break;
		
		case TOK_START_PARENTH:
			putc('(', f);
			break;
		case TOK_END_PARENTH:
			putc(')', f);
			break;
		
		case TOK_DO:
			fputs("do", f);
			break;
		case TOK_END:
			fputs("end", f);
			break;
		
		case TOK_END_EXPR:
			fputs("Expr-end (',' | \\n)", f);
			break;
			
		case TOK_END_OF_STREAM:
			fputs("EOF", f);
			break;
		
		default:
			fprintf(f, "Unkown token (%u)", (unsigned) tok.type);
			break;
	}
}

void fmt_print_line_ref(FILE *f, const char *src, const char *at, size_t at_len, const char *src_name) {
	const char *line_start = src;
	unsigned long long line_count = 1;
	unsigned int n_spaces = 0, n_tabs = 0;
	
	const char *at_start;
	for(at_start = src; *at_start != '\0' && at_start != at; at_start++) {
		if(*at_start == '\n') {
			line_start = at_start + 1;
			line_count++;
			n_spaces = 0;
			n_tabs = 0;
		} else if(*at_start == '\t') {
			n_tabs++;
		} else {
			n_spaces++;
		}
	}
	
	if(at_start != at) {
		fputs("Unable to find in source\n", f);
		return;
	}
	
	fprintf(f, "In %s:%llu, %u\n", src_name, line_count, n_spaces + n_tabs);
	print_line(f, line_start);
	putc('\n', f);
	print_n_chars(f, '\t', n_tabs);
	print_n_chars(f, ' ', n_spaces);
	
	putc('^', f);
	if(at_len > 1) {
		print_n_chars(f, '~', at_len);
	}
	putc('\n', f);
}

void fmt_print_lex_tok(FILE *f, const char *msg, struct lex_token tok) {
	if(msg == NULL) {
		print_tok(f, tok);
		return;
	}
	
	const char *str_start = msg;
	for(const char *c = msg; *c != '\0'; c++) {
		if(*c == '%') {
			if(str_start) {
				print_len_str(f, str_start, c - str_start);
				str_start = NULL;
			}
			print_tok(f, tok);
		} else if(str_start == NULL) {
			str_start = c;
		}
	}
	
	if(str_start) {
		fputs(str_start, f);
	}
}

void fmt_blame_token(FILE *f, const char *msg, struct lex_token tok, const char *src, const char *src_name) {
	fmt_print_lex_tok(f, msg, tok);
	putc('\n', f);
	fmt_print_line_ref(f, src, src + tok.src_start, tok.src_len, src_name);
}


void fmt_print_parse_node(FILE *f, struct parse_node *node) {
	switch(node->type) {
		case PNODE_EXPR: {
			putc('(', f);
			fmt_print_parse_node(f, node->expr.op);
			
			for(unsigned i = 0; i < node->expr.n_args; i++) {
				putc(' ', f);
				fmt_print_parse_node(f, node->expr.args[i]);
			}
			
			putc(')', f);
		} break;
		
		case PNODE_SYM: {
			print_len_str(f, node->str.str, node->str.len);
			fputs(" (Symbol)", f);
			//fprintf(f, "Symbol[%llu]", (unsigned long long) node->sym);
		} break;
		
		case PNODE_VAR: {
			print_len_str(f, node->str.str, node->str.len);
			fputs(" (Variable)", f);
		} break;
		
		case PNODE_INT: {
			fprintf(f, "%lli (Integer)", (long long) node->int_v);
		} break;
		
		default:
			fputs("Unkown", f);
			break;
	}
}

void fmt_blame_parse_node(FILE *f, const char *msg, struct parse_node *node, const char *src, const char *src_name) {
	if(msg == NULL) {
		fmt_print_parse_node(f, node);
		return;
	}
	
	const char *str_start = msg;
	for(const char *c = msg; *c != '\0'; c++) {
		if(*c == '%') {
			if(str_start) {
				print_len_str(f, str_start, c - str_start);
				str_start = NULL;
			}
			fmt_print_parse_node(f, node);
		} else if(str_start == NULL) {
			str_start = c;
		}
	}
	
	if(str_start) {
		fputs(str_start, f);
	}
	putc('\n', f);
	if(src != NULL)
		fmt_print_line_ref(f, src, src + node->src_start, node->src_len, src_name);
	else if(src_name != NULL)
		fprintf(f, "(In %s\n)", src_name);
}

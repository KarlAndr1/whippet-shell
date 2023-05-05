#include "lexer.h"

#include "../proj_utils.h"
#include <string.h>

typedef struct lex_token lex_token;

struct token_buff {
	lex_token *buff, *top, *end;
};

static void token_buff_add(struct token_buff *buff, lex_token tok) {
	if(buff->top == buff->end) {
		size_t len = buff->end - buff->buff;
		size_t new_len = len * 2;
		
		buff->buff = SREALLOC(lex_token, buff->buff, new_len);
		buff->top = buff->buff + len;
		buff->end = buff->buff + new_len;
	}
	
	*(buff->top++) = tok;
}

static void token_buff_init(struct token_buff *buff) {
	size_t cap = 16;
	buff->buff = NSALLOC(lex_token, cap);
	buff->top = buff->buff;
	buff->end = buff->buff + cap;
}

static bool is_whitespace(char c) {
	switch(c) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return true;
		
		default:
			return false;
	}
}

unsigned char get_special_char_token(char c) {
	switch(c) {
		case ',':
		//case '\n':
			return TOK_END_EXPR;

		case '(':
		case '[':
			return TOK_START_PARENTH;
		case ')':
		case ']':
			return TOK_END_PARENTH;
		
		/*
		case '^':
		case '\\':
		case '!':
		case ':':
			return TOK_FUNCTION; */
		
		case '\'':
		case '$':
		case '@':
			return TOK_SIGIL;
		
		default:
			return TOK_NULL;
	}
}

static bool is_valid_symbol_char(char c) {
	return !is_whitespace(c) && get_special_char_token(c) == TOK_NULL && c != '\n' && c != '\0';
}

static bool is_numeric(char c) {
	return c >= '0' && c <= '9';
}

static const char *parse_int(const char *c, long long *out_int) {
	
	long long int_v = 0;
	for(; is_numeric(*c); c++) {
		int_v *= 10;
		int_v += (*c) - '0';
	}
	
	*out_int = int_v;
	
	return c;
}

static const char *skip_whitespace(const char *c) {
	while(*c != '\0' && is_whitespace(*c))
		c++;
	return c;
}

lex_token *lex_tokenize(const char *src, memory_region *region) {
	struct token_buff out_buff; token_buff_init(&out_buff);
	
	const char *c = src;
	
	while(*c != '\0') {
		
		if(*c == '\n') {
			lex_token end_expr = { .type = TOK_END_EXPR, .src_start = c - src, .src_len = 1 };
			token_buff_add(&out_buff, end_expr);
			c = skip_whitespace(c);
			continue;
		}
		
		if(is_whitespace(*c)) {
			c++;
			continue;
		}
		
		if(*c == '#') {
			c++;
			while(*c != '\n' && *c != '\0' && *c != '#') {
				c++;
			}
			if(*c == '#')
				c++;
			continue;
		}
		
		unsigned char special;
		if( (special = get_special_char_token(*c)) != TOK_NULL ) {
			token_buff_add(&out_buff, (lex_token) { .type = special, .src_start = c - src, .src_len = 1 } );
			c++;
			continue;
		}
		
		if(*c == '"') { //String literal
			c++;
			const char *str_start = c;
			while(*c != '"' && *c != '\0') {
				c++;
			}
			const char *str_end = c;
			
			c++;
			
			char *str_cpy = nralloc(region, (str_end - str_start), char);
			memcpy(str_cpy, str_start, str_end - str_start);
			
			lex_token str_tok = { .src_start = str_start - src, .src_len = str_end - str_start, .type = TOK_SYMBOL };
			str_tok.str = (lstring) { str_cpy, str_end - str_start };
			
			token_buff_add(&out_buff, str_tok);
			continue;
			//S_ASSERT(false);
		}
		
		if(is_numeric(*c)) { //Int/Float literal
			long long int_v;
			
			const char *num_start = c;
			c = parse_int(c, &int_v);
			
			lex_token lit_tok = { .type = TOK_INT_LITERAL, .src_start = num_start - src, .src_len = c - num_start, .int_literal = int_v };
			
			if(*c == '.') { // Float
				//double float_v = int_v;
				c++;
				
				lit_tok.type = TOK_FLOAT_LITERAL;
				S_ASSERT(false); //Unimplemented
			}
			
			token_buff_add(&out_buff, lit_tok);
			
			continue;
			//S_ASSERT(false);
		}
		
		//Otherwise its a symbol/keyword
		const char *symbol_start = c;
		while(is_valid_symbol_char(*c))
			c++;
		lstring sym_str = { symbol_start, c - symbol_start };
		
		if(sym_str.len == 2 && sym_str.str[0] == 'd' && sym_str.str[1] == 'o') {
			token_buff_add(&out_buff, (lex_token) { .src_start = symbol_start - src, .src_len = 2, .type = TOK_DO });
			continue;
		}
		if(sym_str.len == 3 && sym_str.str[0] == 'e' && sym_str.str[1] == 'n' && sym_str.str[2] == 'd') {
			token_buff_add(&out_buff, (lex_token) { .src_start = symbol_start - src, .src_len = 3, .type = TOK_END });
			continue;
		}
		
		lex_token out_tok = { .src_start = symbol_start - src, .src_len = sym_str.len, .type = TOK_SYMBOL };
		char *sym_cpy = nralloc(region, sym_str.len, char);
		memcpy(sym_cpy, sym_str.str, sym_str.len);
		sym_str.str = sym_cpy;
		out_tok.str = sym_str;
		
		token_buff_add(&out_buff, out_tok);
	}
	
	token_buff_add(&out_buff, (lex_token) { .type = TOK_END_OF_STREAM, .src_start = c - src, .src_len = 0 } );
	
	return out_buff.buff;
}



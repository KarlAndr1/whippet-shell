#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

//#include "parser.h"
#include "../proj_defs.h"
#include "../proj_utils.h"

enum {
	TOK_NULL,
	
	TOK_SYMBOL,
	
	TOK_INT_LITERAL,
	TOK_FLOAT_LITERAL,
	TOK_STR_LITERAL,
	
	TOK_END_EXPR,
	
	TOK_START_PARENTH,
	TOK_END_PARENTH,
	
	TOK_DO,
	TOK_END,
	
	TOK_SIGIL,
	
	//TOK_FUNCTION,
	
	TOK_END_OF_STREAM
};

struct lex_token {
	unsigned char type;
	unsigned int src_start, src_len;
	union {
		long long int_literal;
		double float_literal;
		//r_string str;
		lstring str;
	};
};

struct lex_token *lex_tokenize(const char *src, memory_region *region);

#endif

#include "parser.h"
#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include "parser_fmt.h"

#include "../colour_defs.h"

typedef struct parse_node parse_node;

typedef struct parse_context {
	memory_region *region;
	const struct symbol_context *sym;
	const struct lex_token *tokens, *tokens_iter;
	const char *src, *src_name;
} parse_context;

static struct lex_token tok_pop(parse_context *p) {
	struct lex_token tok = *(p->tokens_iter);

	if(p->tokens_iter->type != TOK_END_OF_STREAM)
		p->tokens_iter++;

	return tok;
}

static struct lex_token tok_peek(parse_context *p) {
	return *(p->tokens_iter);
}

static struct lex_token accept_tok(unsigned char type, parse_context *p) {
	if(tok_peek(p).type == type)
		return tok_pop(p);
	return (struct lex_token) { .type = TOK_NULL };
}

/*
static bool expect_tok(unsigned char type, parse_context *p, struct lex_token *opt_out_tok) {
	struct lex_token tok = accept_tok(type, p);
	if(tok.type == TOK_NULL) {
		//fmt_blame_token(stderr, "Unexpected token %, expected ...
		return 0;
	}
	
	if(opt_out_tok != NULL)
		*opt_out_tok = tok;
	
	return 1;
} */

static parse_node *new_node(unsigned char type, parse_context *p, struct lex_token *opt_tok) {
	parse_node *node = ralloc(p->region, parse_node);
	node->type = type;
	
	if(opt_tok != NULL) {
		node->src_start = opt_tok->src_start;
		node->src_len = opt_tok->src_len;
	} else {
		node->src_start = 0;
		node->src_len = 0;
	}

	return node;
}

typedef struct node_list {
	parse_node **nodes;
	size_t cap, len;
} node_list;

static void node_list_init(node_list *list) {
	list->cap = 4;
	list->len = 0;
	
	list->nodes = NSALLOC(parse_node*, list->cap);
}

static void node_list_free(node_list *list) {
	list->cap = 0;
	list->len = 0;
	s_dealloc(list->nodes);
	list->nodes = NULL;
}

static parse_node **node_list_copy(node_list *list, memory_region *to_region) {
	parse_node **copy = nralloc(to_region, list->len, parse_node*);
	memcpy(copy, list->nodes, sizeof(parse_node*) * list->len);
	
	return copy;
}

static void node_list_push(node_list *list, parse_node *node) {
	if(list->len == list->cap) {
		list->cap *= 2;
		list->nodes = SREALLOC(parse_node*, list->nodes, list->cap);
	}
	
	list->nodes[list->len++] = node;
}

static void unexpected_token(parse_context *context, struct lex_token tok) {	
	fmt_blame_token(stderr, COLOUR_ERROR "Unexpected token: %" COLOUR_RESET, tok, context->src, context->src_name);
}

static bool can_continue_expression(parse_context *context) {
	struct lex_token tok = tok_peek(context);
	switch(tok.type) {
		
		case TOK_END_PARENTH:
		case TOK_END_EXPR:
		case TOK_END_OF_STREAM:
		case TOK_END:
			return false;
		
		default:
			return true;
	}
}

static parse_node *parse_expr(parse_context *context);

static parse_node *parse_term(parse_context *context) {
	struct lex_token tok = tok_pop(context);
	
	switch(tok.type) {
		
		case TOK_START_PARENTH: {
			parse_node *op = parse_term(context);
			if(op == NULL)
				return NULL;
			
			//accept_tok(TOK_END_EXPR, context);
			
			//if(accept_tok(TOK_END_PARENTH, context).type != TOK_NULL)
			//	return op;
			
			node_list args; node_list_init(&args);
			while(accept_tok(TOK_END_PARENTH, context).type == TOK_NULL) {
				parse_node *arg = parse_term(context);
				if(arg == NULL) {
					node_list_free(&args);
					return NULL;
				}
				node_list_push(&args, arg);
				//accept_tok(TOK_END_EXPR, context);
			}
			
			parse_node *expr = new_node(PNODE_EXPR, context, NULL);
			expr->expr.args = node_list_copy(&args, context->region);
			expr->expr.n_args = args.len;
			
			node_list_free(&args);
			
			expr->expr.op = op;
			
			return expr;
			
		}
		
		case TOK_DO: {
			accept_tok(TOK_END_EXPR, context);
			node_list exprs; node_list_init(&exprs);
			while(accept_tok(TOK_END, context).type == TOK_NULL) {
				parse_node *expr = parse_expr(context);
				if(expr == NULL) {
					node_list_free(&exprs);
					return NULL;
				}
				node_list_push(&exprs, expr);
				accept_tok(TOK_END_EXPR, context);
			}
			
			parse_node *block = new_node(PNODE_BLOCK, context, NULL);
			block->expr.args = node_list_copy(&exprs, context->region);
			block->expr.n_args = exprs.len;
			
			node_list_free(&exprs);
			
			return block;
		}

		case TOK_SYMBOL: {
			parse_node *node = new_node(PNODE_SYM, context, &tok);
			node->str = tok.str;
			return node;
		}
		
		case TOK_INT_LITERAL: {
			parse_node *node = new_node(PNODE_INT, context, &tok);
			node->int_v = tok.int_literal;
			return node;
		}
		
		case TOK_SIGIL: {
			struct lex_token sym = tok_pop(context);
			if(sym.type != TOK_SYMBOL) {
				fmt_blame_token(stderr, COLOUR_ERROR "Expected symbol following sigil, got '%' instead" COLOUR_RESET, sym, context->src, context->src_name);
				return NULL;
			}
			parse_node *node = new_node(PNODE_VAR, context, &tok);
			node->str = sym.str;
			return node;
		}
		
		default:
			unexpected_token(context, tok);
			return NULL;
	}
}

static parse_node *parse_expr(parse_context *context) {
	
	parse_node *op = parse_term(context);
	
	if(op == NULL)
		return NULL;
	
	node_list args; node_list_init(&args);
	while(can_continue_expression(context)) {
		parse_node *arg = parse_term(context);
		if(arg == NULL) {
			node_list_free(&args);
			return NULL;
		}
		node_list_push(&args, arg);
	}
	if(args.len == 0) {
		node_list_free(&args);
		return op;
	}
	
	parse_node **args_p = node_list_copy(&args, context->region);

	parse_node *expr = new_node(PNODE_EXPR, context, NULL);
	expr->expr.args = args_p;
	expr->expr.n_args = args.len;
	expr->expr.op = op;
	
	node_list_free(&args);
	return expr;
}

struct parse_node *par_parse(const char *src_name, const char *src, memory_region *parse_region, memory_region *sym_region) {
	parse_context context = {
		.region = parse_region,
		.tokens = lex_tokenize(src, sym_region),
		.src = src,
		.src_name = src_name
	};
	
	if(context.tokens == NULL)
		return NULL;
	
	context.tokens_iter = context.tokens;
	
	accept_tok(TOK_END_EXPR, &context); //A script may begin with a leading endline
	
	parse_node *expr = parse_expr(&context);
	
	accept_tok(TOK_END_EXPR, &context);
	
	struct lex_token end_tok = tok_peek(&context);
	
	if(end_tok.type != TOK_END_OF_STREAM && expr != NULL)
		fmt_blame_token(stderr, COLOUR_INFO "Warning: Extra tokens after end of expression (%)" COLOUR_RESET, end_tok, src, src_name);
	
	s_dealloc((struct lex_token*) context.tokens);
	
	return expr;
}

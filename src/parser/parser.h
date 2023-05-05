#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "../proj_utils.h"
#include "../proj_defs.h"

enum {
	PNODE_EXPR,
	PNODE_SYM,
	PNODE_INT,
	PNODE_FLOAT,
	PNODE_STRING,
	PNODE_VAR,
	PNODE_FN,
	PNODE_BLOCK
};

struct parse_node {
	unsigned char type;
	unsigned int src_start, src_len;
	union {
		struct {
			struct parse_node *op;
			unsigned n_args;
			struct parse_node **args;
		} expr;
		/*struct {
			unsigned n_args;
			lstring *args;
			struct parse_node *expr;
		} fn; */
		long long int_v;
		double float_v;
		lstring str;
	};
};

struct parse_node *par_parse(const char *src_name, const char *src, memory_region *parse_region, memory_region *sym_region);

#endif

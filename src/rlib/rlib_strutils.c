#include "rlib_strutils.h"

#include "rlib.h"

#include "../proj_utils.h"

#include <string.h>

DECL_R_OP(endswith) {
	if(args[0].type != TYPE_STR || args[1].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };
	
	if(args[1].str_v->len > args[0].str_v->len)
		return (struct r_val) { .type = TYPE_INT, .int_v = 0 }; //String A cant end with string B if len(B) > len(A)
	
	for(unsigned i = args[0].str_v->len - args[1].str_v->len, c = 0; i < args[0].str_v->len; (void) (i++ && c++)) {
		if(args[0].str_v->str[i] != args[1].str_v->str[c])
			return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = 1 };
}

static int cmp_r_strings(const struct r_string *a, const struct r_string *b, unsigned a_i) {
	S_ASSERT(a_i < a->len);
	if(a->len - a_i < b->len)
		return 0;
	
	for(unsigned i = 0; i < b->len; i++) {
		if(a->str[a_i + i] != b->str[i])
			return 0;
	}
	
	return 1;
}

static struct r_string *r_string_substr(const struct r_string *str, unsigned start, unsigned len) {
	S_ASSERT(start + len <= str->len);
	
	struct r_string *substr = s_alloc(sizeof(struct r_string *) + len);
	memcpy( (char *) substr->str, str->str + start, len);
	substr->len = len;
	substr->ref_c = 1;
	
	return substr;
}

static int is_whitespace(char c) {
	switch(c) {
		
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return 1;
		
		default:
			return 0;
	}
}

DECL_R_OP(split) {
	
	for(unsigned a = 0; a < n_args; a++) {
		if(args[a].type != TYPE_STR)
			return (struct r_val) { .type = TYPE_NULL };
	}
	
	struct { struct r_string **strs; unsigned len, cap; } str_buff = { .len = 0, .cap = 4 };
	str_buff.strs = NSALLOC(struct r_string*, str_buff.cap);
	
	unsigned i = 0;
	//const char *str_start = args[0].str_v->str;
	unsigned str_start = 0;
	
	while(i < args[0].str_v->len) {
		
		bool matched = false;
		for(unsigned j = 1; j < n_args; j++) {
			if(cmp_r_strings(args[0].str_v, args[j].str_v, i)) {
				
				if(str_start != i) { /*
					struct r_string *substr = s_alloc(sizeof(struct r_string) + i - str_start);
					substr->len = i - str_start;
					memcpy( (char *) substr->str, args[0].str_v->str + str_start, substr->len); */
					struct r_string *substr = r_string_substr(args[0].str_v, str_start, i - str_start);
					
					if(str_buff.len == str_buff.cap) {
						str_buff.cap *= 2;
						str_buff.strs = SREALLOC(struct r_string*, str_buff.strs, str_buff.cap);
					}
					str_buff.strs[str_buff.len++] = substr;
				}
				
				i += args[j].str_v->len;
				str_start = i;
				
				matched = true;
				break;
			}
		}
		if(!matched)
			i++;
	}
	
	if(str_start < args[0].str_v->len) {
		struct r_string *substr = r_string_substr(args[0].str_v, str_start, args[0].str_v->len - str_start);
		
		if(str_buff.len == str_buff.cap) {
			str_buff.cap += 1;
			str_buff.strs = SREALLOC(struct r_string*, str_buff.strs, str_buff.cap);
		}
		str_buff.strs[str_buff.len++] = substr;
	}
	
	struct r_array *res = s_alloc(sizeof(struct r_array) + sizeof(struct r_val) * str_buff.len);
	res->len = str_buff.len;
	res->ref_c = 1;
	
	for(unsigned i = 0; i < str_buff.len; i++) {
		res->items[i] = (struct r_val) { .type = TYPE_STR, .str_v = str_buff.strs[i] };
	}
	
	s_dealloc(str_buff.strs);
	
	return (struct r_val) { .type = TYPE_ARRAY, .array_v = res };
}

DECL_R_OP(trim) {
	if(args[0].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };
	
	const struct r_string *str = args[0].str_v;
	
	unsigned start, end;
	for(start = 0; start < str->len; start++) {
		if(!is_whitespace(str->str[start]))
			break;
	}
	
	for(end = str->len; end > 0; end--) {
		if(!is_whitespace(str->str[end - 1]))
			break;
	}
	
	//S_ASSERT(end >= start);
	
	unsigned len;
	
	if(end > start) {
		len = end - start;
	} else {
		len = 0;
	}
	
	struct r_string *res = r_string_substr(str, start, len);
	
	return (struct r_val) { .type = TYPE_STR, .str_v = res };
}

DECL_R_OP(contains) {
	if(args[0].type != TYPE_STR)
		return (struct r_val) { .type = TYPE_NULL };
	
	for(unsigned i = 1; i < n_args; i++) {
		if(args[i].type != TYPE_STR)
			return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
	}
	
	const struct r_string *str = args[0].str_v;
	
	for(unsigned i = 0; i < str->len; i++) {
		for(unsigned j = 1; j < n_args; j++) {
			if(cmp_r_strings(str, args[j].str_v, i))
				return (struct r_val) { .type = TYPE_INT, .int_v = 1 };
		}
	}
	
	return (struct r_val) { .type = TYPE_INT, .int_v = 0 };
}

static struct rlib_op ops[] = {
	DEF_R_OP(endswith, "endswith", 2),
	DEF_R_OP(split, "split", -3),
	DEF_R_OP(trim, "trim", 1),
	DEF_R_OP(contains, "contains", -3)
};

static char loaded = 0;

void rlib_strutils_load() {
	if(loaded)
		return;
	
	loaded = 1;
	LOAD_RLIB(ops);
}

void rlib_strutils_put(struct interp_env *env) {
	PUT_RLIB(ops, env);
}

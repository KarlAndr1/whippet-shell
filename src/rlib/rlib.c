#include "rlib.h"

int r_val_as_bool(struct r_val val) {
	switch(val.type) {
		
		case TYPE_INT:
			return val.int_v != 0;
		
		case TYPE_STR:
			return val.str_v->len != 0;
		
		default:
			return 0;
	}
}

int cmp_r_vals(struct r_val a, struct r_val b) {
	if(a.type != b.type)
		return 0;
	
	switch(a.type) {
		case TYPE_INT:
			return a.int_v == b.int_v;
		
		case TYPE_STR:
			if(a.str_v->len != b.str_v->len)
				return 0;
			for(unsigned i = 0; i < a.str_v->len; i++) {
				if(a.str_v->str[i] != b.str_v->str[i])
					return 0;
			}
			return 1;
		
		case TYPE_FN:
			return a.fn == b.fn;
		
		case TYPE_EXT_FN:
			return a.ext_fn == b.ext_fn;
			
		default:
			return false;
	}
}

int r_vals_less(struct r_val a, struct r_val b) {
	if(a.type != b.type)
		return 0;
	
	switch(a.type) {
		case TYPE_INT:
			return a.int_v < b.int_v;
		
		default:
			return 0;
	}
}

int r_vals_greater(struct r_val a, struct r_val b) {
	if(a.type != b.type)
		return 0;
	
	switch(a.type) {
		case TYPE_INT:
			return a.int_v > b.int_v;
		
		default:
			return 0;
	}
}

#include <string.h>

char *r_string_to_cstr(const struct r_string *str) {
	char *res = s_alloc(str->len + 1);
	memcpy(res, str->str, str->len);
	res[str->len] = '\0';
	
	return res;
}

struct r_string *cstr_to_rstring(const char *str) {
	unsigned len = strlen(str);
	struct r_string *res = s_alloc(sizeof(struct r_string) + len);
	res->len = len;
	res->ref_c = 1;
	
	memcpy( (char *) res->str, str, len);
	
	return res;
}

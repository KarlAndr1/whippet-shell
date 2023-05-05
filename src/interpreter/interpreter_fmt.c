#include "interpreter_fmt.h"

void fmt_print_r_val(FILE *f, struct r_val val) {
	switch(val.type) {
		case TYPE_NULL:
			fputs("Null", f);
			break;
		
		case TYPE_INT:
			fprintf(f, "%lli", (long long) val.int_v);
			break;
		
		case TYPE_STR:
			print_len_str(f, val.str_v->str, val.str_v->len); 
			break;
		
		case TYPE_FN:
		case TYPE_EXT_FN:
			fputs("Function", f);
			break;
		
		case TYPE_ARRAY:
			putc('(', f);
			for(unsigned i = 0; i < val.array_v->len; i++) {
				fmt_print_r_val(f, val.array_v->items[i]);
				if(i != val.array_v->len - 1)
					putc(' ', f);
			}
			putc(')', f);
			break;
		
		default:
			fputs("Unkown value", f);
			break;
	}
}

#include <string.h> 

static char *write_to_buff(char *buff, char *buff_end, const char *src, size_t len) {
	if(len > buff_end - buff) {
		return NULL;
	}
	
	memcpy(buff, src, len);
	return buff + len;
}

static char *write_char_to_buff(char *buff, char *buff_end, char c) {
	if(buff < buff_end) {
		*(buff++) = c;
		return buff;
	}
	
	return NULL;
}

char *fmt_write_r_val_to_buff(char *buff, char *buff_end, struct r_val val, bool c_str) {
	switch(val.type) {
		case TYPE_STR:
			buff = write_to_buff(buff, buff_end, val.str_v->str, val.str_v->len);
			if(c_str)
				buff = write_char_to_buff(buff, buff_end, '\0');
			return buff;
		
		case TYPE_INT: {
			int n = snprintf(buff, buff_end - buff, "%lli", (long long) val.int_v);
			if(c_str)
				n++;
			if(n > buff_end - buff)
				return NULL;
			return buff + n;
		}
		
		default:
			if(c_str) {
				if(buff >= buff_end)
					return NULL;
				*buff = '\0';
				return ++buff;
			} else
				return buff;
	}
}

#ifndef PROJ_DEFS_H_INCLUDED
#define PROJ_DEFS_H_INCLUDED

typedef long long r_int;
typedef double r_float;
typedef unsigned long long symbol_i;

struct r_string {
	unsigned ref_c;
	unsigned len;
	const char str[];
};
/*
struct r_list {
	
} */

#define KEYWORD_NULL 0

#endif

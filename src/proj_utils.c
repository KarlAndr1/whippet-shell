#include "proj_utils.h"
#include <stdio.h>
#include <string.h>

void *(*s_alloc)(size_t);
void *(*s_realloc)(void*, size_t);
void (*s_dealloc)(void*);

error_reporter_t main_error;

void s_assert_call(bool c, int line, const char *file) {
	if(!c) {
		printf("Assertion failed in %s:%i\n", file, line);
		main_error(ERR_GENERIC, "Assertion failed!");
	}
}

//-------------- Generic array list

list_g *new_list(size_t cap) {
	S_ASSERT(cap > 0);
	
    list_g *n_list = s_alloc(sizeof(list_g));
    n_list->list = s_alloc(sizeof(void*) * cap);
    n_list->len = 0;
    n_list->cap = cap;
    
    return n_list;
}

void free_list(list_g *list) {
	S_ASSERT(list != NULL);
	
    s_dealloc(list->list);
    s_dealloc(list);
}

void list_append(list_g *list, void *ptr) {
	S_ASSERT(list != NULL);
	
    if(list->len < list->cap) {
        list->list[list->len] = ptr;
        list->len++;
    } else {
        list->cap *= 2;
        list->list = s_realloc(list->list, list->cap * sizeof(void *)); //Bug, the * sizeof(void *) was missing
        list_append(list, ptr);
    }
}

void *list_pop(list_g *list) {
	S_ASSERT(list != NULL);
	S_ASSERT(list->len > 0);
	
	list->len--;
	return list->list[list->len];
}

void list_clear(list_g *list) {
	S_ASSERT(list != NULL);
	list->len = 0;
}

void *list_get(list_g *list, size_t i) {
	S_ASSERT(list != NULL);
	S_ASSERT(i < list->len);
	return list->list[i];
}

//-------------- Regions

typedef struct memory_block {
	struct memory_block *prev_block;
	size_t len;
	unsigned char data[];
} memory_block;

typedef struct memory_region {
	struct memory_block *block;
	size_t total;
	unsigned char *top;
	unsigned char *end;
} memory_region;

static memory_block *new_memory_block(size_t len, memory_block *prev) {
	memory_block *block = s_alloc(sizeof(memory_block) + sizeof(unsigned char) * len);
	block->len = len;
	block->prev_block = prev;
	
	return block;
}

memory_region *new_memory_region(size_t initial_len) {
	memory_region *region = s_alloc(sizeof(memory_region));
	memory_block *block = new_memory_block(initial_len, NULL);
	
	region->block = block;
	region->top = block->data + initial_len;
	region->end = block->data;
	
	region->total = initial_len;
	
	return region;
}

#define REGION_GROW_FACTOR(l) (l * 3)/2

void *region_alloc(memory_region *region, size_t n) {
	S_ASSERT(region != NULL);
	
	region->top -= n;
	if(region->top >= region->end)
		return region->top;
	
	//size_t n_len = region->block->len * 2;
	size_t n_len = REGION_GROW_FACTOR(region->block->len);
	
	memory_block *n_block = new_memory_block(n_len, region->block);
	region->block = n_block;
	
	region->end = n_block->data;
	region->top = n_block->data + n_len;
	
	region->total += n_len;
	
	return region_alloc(region, n);
}

static void free_memory_block(memory_block *block) {
	S_ASSERT(block != NULL);
	
	memory_block *next = block;
	while(next != NULL) {
		block = next;
		next = block->prev_block;
		s_dealloc(block);
	}
}

void free_memory_region(memory_region *region) {
	S_ASSERT(region != NULL);
	
	free_memory_block(region->block);
	s_dealloc(region);
}

void clear_memory_region(memory_region *region) {
	S_ASSERT(region != NULL);
	
	if(region->block->prev_block != NULL) {
		free_memory_block(region->block->prev_block); //Free all blocks except the newest one
		region->block->prev_block = NULL;
	}
	
	region->top = region->block->data + region->block->len;
	region->total = region->block->len;
}

size_t get_region_size(memory_region *region) {
	S_ASSERT(region != NULL);
	
	return region->total;
}

//---------------------------- Various other utils

bool cmp_len_strs(const char *str1, size_t len1, const char *str2, size_t len2) {
	if(len1 != len2)
		return false;
	
	for(size_t i = 0; i < len1; i++) {
		if(str1[i] != str2[i])
			return false;
	}
	
	return true;
}

bool cmp_equlen_strs(const char *str1, const char *str2, size_t len) {
	for(size_t i = 0; i < len; i++) {
		if(str1[i] != str2[i])
			return false;
	}
	
	return true;
}

void print_len_str(FILE *f, const char *str, size_t len) {
	for(size_t i = 0; i < len; i++) {
		putc(str[i], f);
	}
}

void print_line(FILE *f, const char *str) {
	const char *end;
	for(end = str; *end != '\n' && *end != '\0'; end++);
	fwrite(str, 1, end - str, f);
}

void print_n_chars(FILE *f, char c, unsigned long long n) {
	for(; n != 0; n--) {
		putc(c, f);
	}
}

bool y_or_n_prompt(FILE *out, FILE *in, const char *msg) {
	START:
	if(msg)
		fprintf(out, "%s (y/n):", msg);
	else
		fputs("(y/n):", out);
	
	char initial_c = getchar();
	while(getchar() != '\n');
	
	switch(initial_c) {
		case 'y':
		case 'Y':
			return true;
		
		case 'n':
		case 'N':
			return false;
		
		default:
			goto START;
	}
}

char *read_alloc_file(FILE *f, bool cstr) {
	char *buff, *top, *end;
	
	buff = s_alloc(16);
	top = buff;
	end = buff + 16;
	
	size_t res;
	while ( (res = fread(top, sizeof(char), end - top, f)) != 0) {
		top += res;
		if(top == end) {
			size_t cap = end - buff;
			buff = s_realloc(buff, cap * 2);
			top = buff + cap;
			end = buff + cap * 2;
		}
	}
	
	if(cstr) {
		if(top == end) {
			size_t cap = end - buff;
			buff = s_realloc(buff, cap + 1);
			buff[cap] = '\0';
		} else {
			*top = '\0';
		}
	}
	
	return buff;
}

//---------------------------- lstring

bool lstring_cmp(lstring *a, lstring *b) {
	if(a->len != b->len)
		return false;
	
	const char *ap = a->str, *bp = b->str;
	for(size_t i = 0; i < a->len; ++i) {
		if(*ap != *bp)
			return false;
		++bp;
		++ap;
	}
	return true;
}

char *lstring_to_cstr(lstring str, memory_region *opt_region) {
	char *cstr;
	if(opt_region)
		cstr = region_alloc(opt_region, str.len + 1);
	else
		cstr = s_alloc(str.len + 1);
	
	memcpy(cstr, str.str, str.len);
	cstr[str.len] = '\0';
	return cstr;
}

void proj_utils_init(error_reporter_t main_err, void *(*n_alloc)(size_t), void *(*n_realloc)(void*, size_t), void (*n_free)(void*)) {
	main_error = main_err;
	
	s_alloc = n_alloc;
	s_realloc = n_realloc;
	s_dealloc = n_free;
}

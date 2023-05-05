#ifndef PROJ_UTILS_H_INCLUDED
#define PROJ_UTILS_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef void(*error_reporter_t)(int error_c, const char *error_msg);

extern error_reporter_t main_error;

void proj_utils_init(error_reporter_t main_err, void *(*n_alloc)(size_t), void *(*n_realloc)(void*, size_t), void (*n_free)(void*));

extern void *(*s_alloc)(size_t);
extern void *(*s_realloc)(void*, size_t);
extern void (*s_dealloc)(void*);

#define SALLOC(t) ((t*) s_alloc(sizeof(t)))
#define NSALLOC(t, n) ((t*) s_alloc(sizeof(t) * (n)))
#define SREALLOC(t, p, n) ((t*) s_realloc(p, sizeof(t) * (n)))

#ifndef NO_INCLUDE_ASSERTS
	void s_assert_call(bool c, int line, const char *file);
	#define S_ASSERT(c) s_assert_call(c, __LINE__, __FILE__)
	#define IF_ASSERTS(b) b
#else
	#define S_ASSERT(c)
	#define IF_ASSERTS(b)
#endif


enum {
	ERR_GENERIC,
	ERR_OUT_OF_MEM,
	ERR_LEXING,
	ERR_PARSING,
	ERR_SEMANTIC,
	ERR_CODEGEN,
	ERR_STATIC_OUT_OF_BOUNDS,
};

//typedef struct memory_block memory_block;
typedef struct memory_region memory_region;

typedef struct list_g {
    void **list;
    size_t len, cap;
} list_g;

typedef struct lstring {
	const char *str;
	size_t len;
} lstring;

#define DEFAULT_LIST_CAP 32

list_g *new_list(size_t cap);
void free_list(list_g *list);
void list_append(list_g *list, void *ptr);
void list_clear(list_g *list);
void *list_pop(list_g *list);
void *list_get(list_g *list, size_t i);

#define NEW_LIST() new_list(DEFAULT_LIST_CAP)

#define DEFAULT_REGION_SIZE 128

memory_region *new_memory_region(size_t initial_len);
void *region_alloc(memory_region *region, size_t n);
void free_memory_region(memory_region *region);
void clear_memory_region(memory_region *region);

size_t get_region_size(memory_region *region);

#define NEW_REGION() new_memory_region(DEFAULT_REGION_SIZE)
#define ralloc_raw(r, n) region_alloc(r, n)
#define nralloc(r, n, s) ( (s*) region_alloc(r, (n) * sizeof(s)) )
#define ralloc(r, t) ( (t*) region_alloc(r, sizeof(t)) )

bool cmp_len_strs(const char *str1, size_t len1, const char *str2, size_t len2);
bool cmp_equlen_strs(const char *str1, const char *str2, size_t len);

void print_len_str(FILE *f, const char *str, size_t len);

void print_line(FILE *f, const char *str);
void print_n_chars(FILE *f, char c, unsigned long long n);

#define LSTRING(strv) (lstring) { .str = strv, .len = sizeof(strv) - 1 }

bool lstring_cmp(lstring *a, lstring *b);

char *lstring_to_cstr(lstring str, memory_region *opt_region);

bool y_or_n_prompt(FILE *out, FILE *in, const char *msg);

char *read_alloc_file(FILE *f, bool cstr);

//Macro utils

#define LENOF(a) (sizeof(a)/sizeof(a[0]))
#define ABS(v) ((v) < 0 ? -(v) : (v))

#endif

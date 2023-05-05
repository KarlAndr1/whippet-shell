#include "../proj_utils.h"

#include "../tui/utf8.h"

static void test1() {
	const char *str1 = "λåð";
	const char *str1_end = str1 + sizeof("λåð") - 1;
	
	S_ASSERT(utf8_count_chars(str1, str1_end) == 3);
	
	const char *str2 = "x";
	const char *str2_end = str2 + 1;
	
	S_ASSERT(utf8_count_chars(str2, str2_end) == 1);
}

static void test2() {
	const char *str1 = "ð";
	S_ASSERT(utf8_get_char_end(str1, str1 + 2) == str1 + 1);
}

void do_utf8_tests() {
	test1();
	test2();
}

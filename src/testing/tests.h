#ifndef TESTS_H_INCLUDED
#define TESTS_H_INCLUDED

void do_tests();

#ifdef NO_TESTS
	#define DO_TESTS()
#else
	#define DO_TESTS() do_tests()
#endif

#endif

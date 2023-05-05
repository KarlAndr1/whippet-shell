#include "utf8.h"

#include <stddef.h>

//https://en.wikipedia.org/wiki/UTF-8#Encoding
const char *utf8_get_char_start(const char *ch, const char *lower_limit_c) {
	const unsigned char *c = (const unsigned char *) ch;
	const unsigned char *lower_limit = (const unsigned char *) lower_limit_c;
	
	if((*c & 128) == 0) //Simple ascii character
		return (const char*) c;
	
	c--;
	if(c < lower_limit) //Cut off character
		return NULL;
	
	if(*c >> 5 == 6) //0b110
		return (const char*) c;
	else if(*c >> 6 != 2)
		return NULL;
	
	c--;
	if(c < lower_limit)
		return NULL;
	
	if(*c >> 4 == 14) //0b1110
		return (const char *) c;
	else if(*c >> 6 != 2)
		return NULL;
	
	c--;
	if(c < lower_limit)
		return NULL;
	
	if(*c >> 3 == 30) //0b11110
		return (const char*) c;
	else
		return NULL; //A 4 character uf8 
}

const char *utf8_get_char_end(const char *ch, const char *end_c) {
	unsigned const char *c = (const unsigned char *) ch;
	const unsigned char *end = (const unsigned char *) end_c;
	
	if((*c & 128) == 0)
		return (const char*) c;
	
	unsigned expected_chars;
	if((*c >> 5) == 6)
		expected_chars = 1;
	else if((*c >> 4) == 14)
		expected_chars = 2;
	else if((*c >> 3) == 30)
		expected_chars = 3;
	else
		return NULL;
	
	for(; expected_chars > 0; expected_chars--) {
		c++;
		if(c >= end)
			return NULL;
		if((*c >> 6) != 2)
			return NULL;
	}
	
	return (const char *) c;
	/*
	c++;
	if(c >= end_c)
		return NULL;
	*/
}

unsigned utf8_count_chars(const char *from, const char *to) {
	const char *c = to - 1;
	unsigned n = 0;
	while(c >= from) {
		const char *start = utf8_get_char_start(c, from);
		if(start == NULL) //Corrupt characters count as 1 character per byte
			c--;
		else
			c = start - 1;
		n++;
	}
	
	return n;
}

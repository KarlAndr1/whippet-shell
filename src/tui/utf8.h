#ifndef UTF8_H_INCLUDED
#define UTF8_H_INCLUDED

const char *utf8_get_char_start(const char *c, const char *lower_limit);

const char *utf8_get_char_end(const char *ch, const char *end_c);

unsigned utf8_count_chars(const char *from, const char *to);

#endif

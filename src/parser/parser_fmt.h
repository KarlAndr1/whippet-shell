#ifndef PARSER_FMT_H_INCLUDED
#define PARSER_FMT_H_INCLUDED

#include "parser.h"
#include "lexer.h"

void fmt_print_line_ref(FILE *f, const char *src, const char *at, size_t at_len, const char *src_name);

void fmt_print_lex_tok(FILE *f, const char *msg, struct lex_token tok);
void fmt_blame_token(FILE *f, const char *msg, struct lex_token tok, const char *src, const char *src_name);

void fmt_print_parse_node(FILE *f, struct parse_node *node);

void fmt_blame_parse_node(FILE *f, const char *msg, struct parse_node *node, const char *src, const char *src_name);

#endif

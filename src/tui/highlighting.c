#include "highlighting.h"

#include <stdbool.h>
#include <stdio.h>

#include "../colour_defs.h"

enum bracket_dir { DIR_LEFT, DIR_RIGHT, DIR_NONE };

static enum bracket_dir get_bracket_dir(char c) {
	switch(c) {
		case '(':
		case '[':
			return DIR_RIGHT;
		
		case ')':
		case ']':
			return DIR_LEFT;
		
		default:
			return DIR_NONE;
	}
}

static bool is_special_char(char c) {
	switch(c) {
		case '@':
		case '$':
		case '\'':
		case '(':
		case ')':
		case '[':
		case ']':
			return true;
		default:
			return false;
	}
}

void highlight_draw_line(const char *line, unsigned len, unsigned cursor_pos) {
	const char *cursor_char = NULL;
	 enum bracket_dir dir = DIR_NONE;
	
	if(cursor_pos < len)
		if( (dir = get_bracket_dir(line[cursor_pos])) != DIR_NONE)
			cursor_char = line + cursor_pos;
	
	if(cursor_char == NULL && cursor_pos > 0)
		if( (dir = get_bracket_dir(line[cursor_pos-1])) != DIR_NONE) 
			cursor_char = line + cursor_pos - 1;
	
	const char *cursor_match = NULL;
	if(cursor_char != NULL) {
		unsigned counter = 0;
		if(dir == DIR_LEFT) {
			for(const char *c = cursor_char - 1; c >= line; c--) {
				enum bracket_dir d = get_bracket_dir(*c);
				if(d == DIR_LEFT)
					counter++;
				else if(d == DIR_RIGHT) {
					if(counter == 0) {
						cursor_match = c;
						break;
					}
					counter--;
				}
			}
		} else if(dir == DIR_RIGHT) {
			for(const char *c = cursor_char + 1; c < line + len; c++) {
				enum bracket_dir d = get_bracket_dir(*c);
				if(d == DIR_RIGHT)
					counter++;
				else if(d == DIR_LEFT) {
					if(counter == 0) {
						cursor_match = c;
						break;
					}
					counter--;
				}
			}
		}
	}
	
	for(const char *c = line; c < line + len; c++) {
		if(c == cursor_match || c == cursor_char) {
			printf(COLOUR_THEME1 "%c" COLOUR_RESET, *c);
			continue;
		}
		if(is_special_char(*c)) {
			printf(COLOUR_THEME2 "%c" COLOUR_RESET, *c);
			continue;
		}
		putchar(*c);
	}
}

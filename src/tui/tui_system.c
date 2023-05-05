#include "tui_system.h"

#include "../proj_utils.h"

#include <unistd.h>
#include <termios.h>

#include "utf8.h"

static int init_done = 0;
struct termios old_settings;

int tui_init() {
	if(init_done != 0)
		return -2;
		
	//Partially based on mini_tui.c, an earlier project
	struct termios term_settings;
	if(tcgetattr(STDIN_FILENO, &term_settings) == -1)
		return -1;
	
	old_settings = term_settings;
	
	cfmakeraw(&term_settings);
	
	if(tcsetattr(STDIN_FILENO, TCSANOW, &term_settings) == -1)
		return -3;
	
	init_done = 1;
	return 0;
}

void tui_deinit() {
	if(init_done == 0)
		return;
	
	tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
	
	init_done = 0;
}

struct history_entry {
	const char *entry;
};

struct tui_state {
	struct history_entry *history;
	int history_current, history_start, history_end;
	unsigned max_history;
	const char *line;
	void (*draw_line)(const char *msg, const char *str, unsigned len, unsigned cursor_pos);
};

struct tui_state *new_tui_state(unsigned max_history, void (*draw_line)(const char *, const char *, unsigned, unsigned)) {
	struct tui_state *state = SALLOC(struct tui_state);
	state->max_history = max_history;
	state->line = NULL;
	state->history = NSALLOC(struct history_entry, max_history);
	
	state->history_current = 0;
	state->history_start = 0;
	state->history_end = 0;
	
	state->draw_line = draw_line;
	
	return state;
}

void free_tui_state(struct tui_state *state) {
	for(int i = state->history_start; i < state->history_end; i++) {
		s_dealloc( (char *) state->history[i % state->max_history].entry);
	}
	s_dealloc(state->history);
	s_dealloc(state);
}

#include <string.h>

int tui_state_history_up(struct tui_state *state) {
	if(state->history_current > state->history_start + 1) {
		state->history_current--;
		return 1;
	}
	return 0;
}

int tui_state_history_down(struct tui_state *state) {
	if(state->history_current < state->history_end) {
		state->history_current++;
		return 1;
	}
	return 0;
}

char read_buff[1024];

const char *tui_state_get_contents(struct tui_state *state) {
	if(state->history_end - state->history_start == 0)
		return NULL;
	
	S_ASSERT(state->history_current > state->history_start);
	return state->history[(state->history_current - 1) % state->max_history].entry;
}

static void shift_buff_left(char *start, char *end, unsigned amount) {
	for(char *c = start + amount; c < end; c++) {
		*(c-amount) = *c;
	}
}

static void shift_buff_right(char *start, char *end) {
	for(char *c = end - 2; c >= start; c--) {
		*(c + 1) = *c;
	}
}

static void add_to_state(struct tui_state *state, const char *line, unsigned len) {
	char *new_entry;
	
	if(line == NULL) {
		new_entry = NULL;
	} else {
		new_entry = s_alloc(len + 1);
		memcpy(new_entry, line, len);
		new_entry[len] = '\0';
	}
	
	state->history_current = state->history_end;
	if(state->history_end - state->history_start == state->max_history) {
		s_dealloc( (char*) state->history[state->history_start % state->max_history].entry);
		state->history_start++;
	}
	state->history[state->history_current % state->max_history].entry = new_entry;
	state->history_current++;
	state->history_end++;
}

int tui_readline(struct tui_state *state, const char *msg) {
	S_ASSERT(init_done);
	
	unsigned cursor_pos = 0;
	unsigned line_end = 0;
		
	bool at_bottom_of_history = true;
	
	int r;
	while(1) {
		putchar('\r');
		state->draw_line(msg, read_buff, line_end, cursor_pos);
		fputs("\x1B[K", stdout); //Erase everything on the line after the newly drawn line

		unsigned n_chars_to_cursor = utf8_count_chars(read_buff + cursor_pos, read_buff + line_end);
		if(n_chars_to_cursor)
			printf("\x1B[%uD", n_chars_to_cursor);
		
		r = getchar();
		if(r == '\n' || r == '\r')
			break;
		if(r == EOF)
			break;
		
		S_ASSERT(cursor_pos <= line_end);
		
		if(r == '\b' || r == 127) { //127 is the delete code in ASCII
			if(cursor_pos > 0) {
				unsigned char_len = 1;
				const char *char_start = utf8_get_char_start(read_buff + cursor_pos - 1, read_buff);
				if(char_start != NULL)
					char_len = (read_buff + cursor_pos) - char_start;
				shift_buff_left((char*) char_start, read_buff + line_end, char_len);
				cursor_pos -= char_len;
				line_end -= char_len;
			}
			continue;
		}
		
		if(r == '\x1B' && getchar() == '[') {
			const char *new_current = NULL;
			switch(getchar()) {
				case 'C': //Forward
					if(cursor_pos < line_end) {
						const char *char_end = utf8_get_char_end(read_buff + cursor_pos, read_buff + line_end);
						if(char_end == NULL)
							cursor_pos++;
						else {
							cursor_pos = char_end - read_buff + 1;
						}
						S_ASSERT(cursor_pos <= line_end);
					}
					break;
				case 'D': //Back
					if(cursor_pos > 0) {
						const char *char_start = utf8_get_char_start(read_buff + cursor_pos - 1, read_buff);
						if(char_start == NULL)
							cursor_pos--;
						else {
							S_ASSERT(char_start >= read_buff);
							cursor_pos = char_start - read_buff;
						}
					}
					break;
				
				case 'A': //Up
					if(at_bottom_of_history) {
						at_bottom_of_history = false;
						new_current = tui_state_get_contents(state);
					} else if(tui_state_history_up(state))
						new_current = tui_state_get_contents(state);

					goto CHANGE_HISTORY;
				case 'B': //Down
					if(tui_state_history_down(state))
						new_current = tui_state_get_contents(state);
					else {
						new_current = "";
						at_bottom_of_history = true;
					}
					goto CHANGE_HISTORY;
			}
			continue;
			
			CHANGE_HISTORY: 
			{
				if(new_current == NULL)
					continue;
				unsigned len = strlen(new_current);
				memcpy(read_buff, new_current, len);
				cursor_pos = len;
				line_end = len;
				continue;
			}
		}
		
		if(line_end == LENOF(read_buff) - 1) {
			while(r != '\n' && r != EOF && r != '\r')
				r = getchar();
			break;
		}
		
		line_end++;
		shift_buff_right(read_buff + cursor_pos, read_buff + line_end);
		read_buff[cursor_pos] = r;
		cursor_pos++;
	}
	fputs("\r\n", stdout);

	if(line_end == 0 && r == EOF)
		return -1;
	
	if(line_end != 0)
		add_to_state(state, read_buff, line_end);
	return 0;
}


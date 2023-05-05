#ifndef TUI_SYSTEM_H_INCLUDED
#define TUI_SYSTEM_H_INCLUDED

int tui_init();
void tui_deinit();

struct tui_state;

struct tui_state *new_tui_state(unsigned max_history, void (*draw_line)(const char *, const char *, unsigned, unsigned));
void free_tui_state(struct tui_state *state);

const char *tui_state_get_contents(struct tui_state *state);

int tui_state_history_up(struct tui_state *state);
int tui_state_history_down(struct tui_state *state);

int tui_readline(struct tui_state *state, const char *msg);

#endif

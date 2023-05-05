#ifndef INTERPRETER_CONFIG_H_INCLUDED
#define INTERPRETER_CONFIG_H_INCLUDED

#include <stdbool.h>

struct int_config {
	bool user_approve_commands;
};

const struct int_config *interpreter_get_config();
void interpreter_set_config(const struct int_config *config);

#endif

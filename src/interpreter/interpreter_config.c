#include "interpreter_config.h"

#include "../proj_utils.h"

static struct int_config int_config_state;
static bool locked = false;

const struct int_config *interpreter_get_config() {
	S_ASSERT(locked);
	return &int_config_state;
}

void interpreter_set_config(const struct int_config *config) {
	if(locked)
		return;	

	int_config_state = *config;
	locked = true;
}

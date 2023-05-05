#ifndef INTERPRETER_FMT_H_INCLUDED
#define INTERPRETER_FMT_H_INCLUDED

#include "interpreter.h"
#include <stdio.h>

void fmt_print_r_val(FILE *f, struct r_val val);

char *fmt_write_r_val_to_buff(char *buff, char *buff_end, struct r_val val, bool c_str);

#endif

#ifndef INTERPRETER_UTILS_H_INCLUDED
#define INTERPRETER_UTILS_H_INCLUDED

#define INT_PRINTERR(env, src_name, msg, ...) int_printerr( env, msg, (struct r_val []) { __VA_ARGS__ }, sizeof( (struct r_val []) { __VA_ARGS__ } ) / sizeof(struct r_val), src_name )

#endif

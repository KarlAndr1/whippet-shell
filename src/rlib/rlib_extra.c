#include "rlib_extra.h"

#include "rlib.h"

DECL_OP(lets) {
	struct r_val assign_val = int_eval_expr(args[1], env, src_name);
	if(assign_val.type != TYPE_ARRAY)
		goto ERR;
	
	struct parse_node *vars = args[0];
	
	if(vars->type != PNODE_EXPR)
		goto ERR;
	
	if(vars->expr.op->type != PNODE_SYM)
		goto ERR;
	
	if(vars->expr.n_args + 1 != assign_val.array_v->len)
		goto ERR;
	
	for(unsigned i = 0; i < vars->expr.n_args; i++) {
		if(vars->expr.args[i]->type != PNODE_SYM)
			goto ERR;
	}
	
	int_env_set(env, vars->expr.op->str, assign_val.array_v->items[0], 1, 0);
	
	for(unsigned i = 0; i < vars->expr.n_args; i++) {
		lstring var_sym = vars->expr.args[i]->str;
		int_env_set(env, var_sym, assign_val.array_v->items[i+1], 1, 0);
	}
	
	return assign_val;
	
	ERR:
	int_decr_refcount(assign_val);
	return (struct r_val) { .type = TYPE_NULL };
}

static struct rlib_op ops[] = {
	DEF_OP(lets, "lets", 2)
};

static char loaded = 0;

void rlib_extra_load() {
	if(loaded)
		return;
	
	loaded = 1;
	LOAD_RLIB(ops);
}

void rlib_extra_put(struct interp_env *env) {
	PUT_RLIB(ops, env);
}

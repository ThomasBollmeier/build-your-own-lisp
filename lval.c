#include "lval.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef void (*operator_assign_fn_t) (Lval* lhs, Lval* rhs, Lval** error);

Lval* lval_num(long n) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_NUM;
	v->data.num = n;
	return v;
}

Lval* lval_decimal(double x) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_DECIMAL;
	v->data.dec = x;
	return v;
}

Lval* lval_err(const char* msg) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_ERR;
	v->data.err = malloc(strlen(msg) + 1);
	strcpy(v->data.err, msg);
	return v;
}

Lval* lval_sym(const char* symbol) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_SYM;
	v->data.sym = malloc(strlen(symbol) + 1);
	strcpy(v->data.sym, symbol);
	return v;
}

Lval* lval_sexpr(void) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_SEXPR;
	v->data.sexpr.count = 0;
	v->data.sexpr.cell = NULL;
	return v;
}

void lval_del(Lval* v) {

	switch(v->type) {
	case LVAL_NUM:
	case LVAL_DECIMAL:
		break;
	case LVAL_SYM:
		free(v->data.sym);
		break;
	case LVAL_ERR:
		free(v->data.err);
		break;
	case LVAL_SEXPR:
		if (v->data.sexpr.cell != NULL) {
			for (int i=0; i<v->data.sexpr.count; i++) {
				lval_del(v->data.sexpr.cell[i]);
			}
			free(v->data.sexpr.cell);
		}
		break;
	}

	free(v);
}

Lval* lval_add(Lval* v, Lval* item) {

	v->data.sexpr.count++;
	v->data.sexpr.cell = (Lval**) realloc(v->data.sexpr.cell, sizeof(Lval*) * v->data.sexpr.count);
	v->data.sexpr.cell[v->data.sexpr.count-1] = item;

	return v;
}

Lval* lval_pop(Lval* v, int i) {

	Lval* ret = v->data.sexpr.cell[i];
	// shift memory:
	memmove(v->data.sexpr.cell+i, v->data.sexpr.cell+i+1, sizeof(Lval*) * (v->data.sexpr.count - i - 1));
	v->data.sexpr.count--;
	v->data.sexpr.cell = (Lval**) realloc(v->data.sexpr.cell, sizeof(Lval*) * v->data.sexpr.count);

	return ret;
}

Lval* lval_take(Lval* v, int i) {

	Lval* ret = lval_pop(v, i);
	lval_del(v);

	return ret;
}

void lval_print(Lval* v) {

	switch(v->type) {
	case LVAL_NUM:
		printf("%li", v->data.num);
		break;
	case LVAL_DECIMAL:
		printf("%lf", v->data.dec);
		break;
	case LVAL_SYM:
		printf("%s", v->data.sym);
		break;
	case LVAL_ERR:
		printf("%s", v->data.err);
		break;
	case LVAL_SEXPR:
		lval_expr_print(v, '(', ')');
		break;
	}

}

void lval_println(Lval* v) {
	lval_print(v);
	putchar('\n');
}

void lval_expr_print(Lval* v, char open, char close) {

	putchar(open);

	for (int i=0; i<v->data.sexpr.count; i++) {
		lval_print(v->data.sexpr.cell[i]);
		if (i != v->data.sexpr.count-1) {
			putchar(' ');
		}
	}

	putchar(close);

}

static void lval_add_assign(Lval* l, Lval* r, Lval** error) {

	switch (l->type) {
	case LVAL_NUM:
		switch (r->type) {
		case LVAL_NUM:
			l->data.num += r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.num += (long) r->data.dec;
			break;
		}
		break;
	case LVAL_DECIMAL:
		switch (r->type) {
		case LVAL_NUM:
			l->data.dec += r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.dec += r->data.dec;
			break;
		}
		break;
	}
}

static void lval_sub_assign(Lval* l, Lval* r, Lval** error) {

	switch (l->type) {
	case LVAL_NUM:
		switch (r->type) {
		case LVAL_NUM:
			l->data.num -= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.num -= (long) r->data.dec;
			break;
		}
		break;
	case LVAL_DECIMAL:
		switch (r->type) {
		case LVAL_NUM:
			l->data.dec -= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.dec -= r->data.dec;
			break;
		}
		break;
	}

}

static void lval_mul_assign(Lval* l, Lval* r, Lval** error) {

	switch (l->type) {
	case LVAL_NUM:
		switch (r->type) {
		case LVAL_NUM:
			l->data.num *= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.num *= (long) r->data.dec;
			break;
		}
		break;
	case LVAL_DECIMAL:
		switch (r->type) {
		case LVAL_NUM:
			l->data.dec *= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.dec *= r->data.dec;
			break;
		}
		break;
	}

}

static void lval_div_assign(Lval* l, Lval* r, Lval** error) {

	int is_zero = 0;

	switch (r->type) {
	case LVAL_NUM:
		is_zero = r->data.num == 0L ? 1 : 0;
		break;
	case LVAL_DECIMAL:
		is_zero = fabs(r->data.dec) < EPS ? 1 : 0;
		break;
	}

	if (is_zero) {
		if (error != NULL) {
			*error = lval_err("Division by zero");
		}
		return;
	}

	switch (l->type) {
	case LVAL_NUM:
		switch (r->type) {
		case LVAL_NUM:
			l->data.num /= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.num /= (long) r->data.dec;
			break;
		}
		break;
	case LVAL_DECIMAL:
		switch (r->type) {
		case LVAL_NUM:
			l->data.dec /= r->data.num;
			break;
		case LVAL_DECIMAL:
			l->data.dec /= r->data.dec;
			break;
		}
		break;
	}

}

static void lval_mod_assign(Lval* l, Lval* r, Lval** error) {

	if (l->type != LVAL_NUM || r->type != LVAL_NUM) {
		if (error != NULL) {
			*error = lval_err("Module operation requires integer numbers");
		}
		return;
	}

	if (r->data.num == 0) {
		if (error != NULL) {
			*error = lval_err("Module operation not defined for zero divisor");
		}
		return;
	}

	l->data.num %= r->data.num;

}

Lval* lval_builtin_op(Lval* v, const char* op) {

	int has_decimals = 0;

	// Ensure that there are only numbers or decimals:
	for (int i=0; i<v->data.sexpr.count; i++) {
		if (v->data.sexpr.cell[i]->type != LVAL_NUM &&
			v->data.sexpr.cell[i]->type != LVAL_DECIMAL) {
			lval_del(v);
			return lval_err("Cannot operate on non-number!");
		} else if (v->data.sexpr.cell[i]->type == LVAL_DECIMAL) {
			has_decimals = 1;
		}
	}

	Lval* ret = NULL;
	Lval* error = NULL;
	operator_assign_fn_t opassign = NULL;

	if (strcmp(op, "+") == 0) {
		ret = !has_decimals ? lval_num(0L) : lval_decimal(0.0);
		opassign = lval_add_assign;
	} else if (strcmp(op, "-") == 0) {
		ret = !has_decimals ? lval_num(0L) : lval_decimal(0.0);
		opassign = lval_sub_assign;
	} else if (strcmp(op, "*") == 0) {
		ret = !has_decimals ? lval_num(1L) : lval_decimal(1.0);
		opassign = lval_mul_assign;
	} else if (strcmp(op, "/") == 0) {
		opassign = lval_div_assign;
	} else if (strcmp(op, "%") == 0) {
		opassign = lval_mod_assign;
	}

	while (v->data.sexpr.count > 0) {

		Lval* x = lval_pop(v, 0);

		if (ret == NULL) {
			if (has_decimals) {
				if (x->type == LVAL_NUM) {
					ret = lval_decimal((double) x->data.num);
				} else {
					ret = lval_decimal(x->data.dec);
				}
			} else {
				ret = lval_num(x->data.num);
			}
			lval_del(x);
			continue;
		}

		opassign(ret, x, &error);

		if (error != NULL) {
			lval_del(x);
			lval_del(ret);
			ret = error;
			break;
		}

		lval_del(x);
	}

	lval_del(v);

	return ret;
}

Lval* lval_eval(Lval* v) {

	return v->type == LVAL_SEXPR ?
		lval_eval_sexpr(v) : v;

}

Lval* lval_eval_sexpr(Lval* v) {

	Lval* first = NULL;

	// Evaluate children:
	for (int i=0; i<v->data.sexpr.count; i++) {
		v->data.sexpr.cell[i] = lval_eval(v->data.sexpr.cell[i]);
		if (v->data.sexpr.cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	switch (v->data.sexpr.count) {
	case 0:
		// Empty expression:
		return v;
	case 1:
		// Single expression:
		return lval_take(v, 0);
	default:
		first = lval_pop(v, 0);
		if (first->type != LVAL_SYM) {
			lval_del(first);
			lval_del(v);
			return lval_err("S-expression does not start with symbol.");
		}
		Lval* ret = lval_builtin_op(v, first->data.sym);
		lval_del(first);
		return ret;
	}
}

Lval* lval_read(mpc_ast_t* t) {

	if (strstr(t->tag, "decimal")) return lval_read_dec(t);

	if (strstr(t->tag, "number")) return lval_read_num(t);

	if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

	// If root or s-expr create list:
	Lval* ret = NULL;
	if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
		ret = lval_sexpr();
	}

	for (int i=0; i<t->children_num; i++) {
		mpc_ast_t* child = t->children[i];
		if (strcmp(child->contents, "(") == 0 ||
			strcmp(child->contents, ")") == 0 ||
			strcmp(child->contents, "{") == 0 ||
			strcmp(child->contents, "}") == 0 ||
			strcmp(child->tag, "regex") == 0)
		{
			continue;
		}
		ret = lval_add(ret, lval_read(child));
	}

	return ret;
}

Lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long value = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
			lval_num(value) :
			lval_err("invalid number");
}

Lval* lval_read_dec(mpc_ast_t* t) {
	double x;
	sscanf(t->contents, "%lf", &x);
	return lval_decimal(x);
}

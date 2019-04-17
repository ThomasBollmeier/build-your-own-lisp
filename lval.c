#include "lval.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Lval* lval_num(long n) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_NUM;
	v->num = n;
	return v;
}

Lval* lval_err(const char* msg) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(msg) + 1);
	strcpy(v->err, msg);
	return v;
}

Lval* lval_sym(const char* symbol) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_SYM;
	v->err = malloc(strlen(symbol) + 1);
	strcpy(v->err, symbol);
	return v;
}

Lval* lval_sexpr(void) {
	Lval* v = (Lval*) malloc(sizeof(Lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(Lval* v) {

	switch(v->type) {
	case LVAL_NUM:
		break;
	case LVAL_SYM:
		free(v->sym);
		break;
	case LVAL_ERR:
		free(v->err);
		break;
	case LVAL_SEXPR:
		if (v->cell != NULL) {
			for (int i=0; i<v->count; i++) {
				lval_del(v->cell[i]);
			}
			free(v->cell);
		}
		break;
	}

	free(v);
}

Lval* lval_add(Lval* v, Lval* item) {

	v->count++;
	v->cell = (Lval**) realloc(v->cell, sizeof(Lval*) * v->count);
	v->cell[v->count-1] = item;

	return v;
}

Lval* lval_pop(Lval* v, int i) {

	Lval* ret = v->cell[i];
	// shift memory:
	memmove(v->cell+i, v->cell+i+1, sizeof(Lval*) * (v->count - i - 1));
	v->count--;
	v->cell = (Lval**) realloc(v->cell, sizeof(Lval*) * v->count);

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
		printf("%li", v->num);
		break;
	case LVAL_SYM:
		printf("%s", v->sym);
		break;
	case LVAL_ERR:
		printf("%s", v->err);
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

	for (int i=0; i<v->count; i++) {
		lval_print(v->cell[i]);
		if (i != v->count-1) {
			putchar(' ');
		}
	}

	putchar(close);

}

Lval* lval_builtin_op(Lval* v, const char* op) {

	// Ensure that there are only numbers:
	for (int i=0; i<v->count; i++) {
		if (v->cell[i]->type != LVAL_NUM) {
			lval_del(v);
			return lval_err("Cannot operate on non-number!");
		}
	}

	Lval* ret = lval_pop(v, 0);

	// Check for negative number
	if (v->count == 0 && strcmp(op, "-") == 0) {
		ret->num = -ret->num;
	}

	while (v->count > 0) {
		Lval* x = lval_pop(v, 0);
		// do operation:
		if (strcmp(op, "+") == 0) {
			ret->num += x->num;
		} else if (strcmp(op, "-") == 0) {
			ret->num -= x->num;
		} else if (strcmp(op, "*") == 0) {
			ret->num *= x->num;
		} else if (strcmp(op, "/") == 0) {
			if (x->num != 0) {
				ret->num /= x->num;
			} else {
				lval_del(ret);
				lval_del(x);
				ret = lval_err("Division by zero");
				break;
			}
		}
		lval_del(x);
	}

	lval_del(v);

	return ret;
}

Lval* lval_eval(Lval* v) {
	return NULL;
}

Lval* lval_eval_sexpr(Lval* v) {

	Lval* first = NULL;

	// Evaluate children:
	for (int i=0; i<v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	switch (v->count) {
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
		Lval* ret = lval_builtin_op(v, first->sym);
		lval_del(first);
		return ret;
	}
}

Lval* lval_expr(Lval* v) {

	return v->type == LVAL_SEXPR ?
		lval_eval_sexpr(v) : v;

}

Lval* lval_read(mpc_ast_t* t) {

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
			strcmp(child->contents, "regex") == 0)
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


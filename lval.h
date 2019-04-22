#ifndef LVAL_H_
#define LVAL_H_

#include "mpc.h"

// ACCURACY
#define EPS 1E-12

// LVAL types:

typedef enum {
	LVAL_ERR,
	LVAL_NUM,
	LVAL_DECIMAL,
	LVAL_SYM,
	LVAL_SEXPR
} LvalType;

struct lval;

typedef struct {
	int count;
	struct lval** cell;
} SexprData;

typedef struct lval {

	LvalType type;
	// type-specific part
	union {
		long num;
		double dec;
		char* err;
		char* sym;
		SexprData sexpr;
	} data;

} Lval;

Lval* lval_num(long n);
Lval* lval_decimal(double x);
Lval* lval_err(const char* msg);
Lval* lval_sym(const char* symbol);
Lval* lval_sexpr(void);
void lval_del(Lval* v);
Lval* lval_add(Lval* v, Lval* item);
Lval* lval_pop(Lval* v, int i);
Lval* lval_take(Lval* v, int i);
void lval_print(Lval* v);
void lval_println(Lval* v);
void lval_expr_print(Lval* v, char open, char close);
Lval* lval_builtin_op(Lval* v, const char* op);
Lval* lval_eval(Lval* v);
Lval* lval_eval_sexpr(Lval* v);
Lval* lval_read(mpc_ast_t* t);
Lval* lval_read_num(mpc_ast_t* t);
Lval* lval_read_dec(mpc_ast_t* t);

#endif /* LVAL_H_ */

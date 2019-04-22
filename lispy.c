#include "mpc.h"

#ifdef _WIN32

const int BUFSIZE = 2048;
static char buffer[BUFSIZE];

char* readline(char* prompt) {

	fputs(prompt, stdout);
	fgets(buffer, BUFSIZE, stdin);

	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = 0;

	return cpy;
}

int add_history(const char* unused) {
	return 0;
}

#else
#include <editline/readline.h>
#endif

#include "lval.h"

int main(void) {

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Decimal = mpc_new("decimal");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
			"number : /-?[0-9]+/;"
			"decimal: /-?[0-9]+\\.[0-9]+/;"
			"symbol : '+' | '-' | '*' | '/' | '%';"
			"sexpr : '(' <expr>* ')';"
			"expr : <decimal> | <number> | <symbol> | <sexpr>;"
			"lispy : /^/ <expr>* /$/;",
			Number, Decimal, Symbol, Sexpr, Expr, Lispy);

	puts("Lispy Version 0.0.0.0.5");
	puts("Press Ctrl+C to Exit\n");

	while(1) {

		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t result;

		if (mpc_parse("<stdin>", input, Lispy, &result)) {
			Lval* value = lval_eval(lval_read(result.output));
			lval_print(value);
			lval_del(value);
		} else {
			mpc_err_print(result.error);
			mpc_err_delete(result.error);
		}

		free(input);

		printf("\n");
	}

	mpc_cleanup(6, Number, Decimal, Symbol, Sexpr, Expr, Lispy);

	return 0;
}

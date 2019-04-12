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

	printf("lispy>...\n");

	return 0;
}

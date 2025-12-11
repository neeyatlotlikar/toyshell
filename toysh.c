#include <stdio.h>
#include <stdlib.h>

int main(void) {
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while (1) {
		printf("toysh> ");
		fflush(stdout);

		nread = getline(&line, &len, stdin);
		if (nread == -1) {
			printf("\nExiting toysh.\n");
			break;
		}

		printf("you typed: %s", line);
	}

	free(line);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TOKENS 128

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

		if (nread > 0 && line[nread - 1] == '\n') {
			line[nread - 1] = '\0';
		}

		char *argv[MAX_TOKENS];
		int argc = 0;

		char *token = strtok(line, " \t");
		while (token != NULL && argc < MAX_TOKENS - 1) {
			argv[argc++] = token;
			token = strtok(NULL, " \t");
		}
		argv[argc] = NULL;

		if (argc == 0) {
			continue;
		}

		printf("argv[0] = %s\n", argv[0]);
		for (int i = 1; i < argc; i++) {
			printf("argv[%d] = %s\n", i, argv[i]);
		}
	}

	free(line);
	return 0;
}

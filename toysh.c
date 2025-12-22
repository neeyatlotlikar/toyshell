#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
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

		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			continue;
		} else if (pid == 0) {
			execvp(argv[0], argv);
			perror("execvp");
			_exit(127);
		} else {
			int status = 0;
			if (waitpid(pid, &status, 0) < 0) {
				perror("waitpid");
			}
		}
	}

	free(line);
	return 0;
}

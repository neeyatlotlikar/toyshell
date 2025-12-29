#include <fcntl.h>
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

		if (strcmp(argv[0], "exit") == 0) {
			printf("Exiting toysh.\n");
			free(line);
			exit(0);
		}

		if (argc > 1 && strcmp(argv[argc - 2], ">") == 0) {
			// argv[0..argc-3] = command+args, argv[argc-1] =
			// filename
			char *cmd_argv[MAX_TOKENS];
			for (int i = 0; i < argc - 2; i++) {
				cmd_argv[i] = argv[i];
			}
			cmd_argv[argc - 2] = NULL;

			pid_t pid = fork();
			if (pid == 0) {
				// Child: open file, redirect stdout
				int fd =
				    open(argv[argc - 1],
					 O_CREAT | O_TRUNC | O_WRONLY, 0644);
				if (fd < 0) {
					perror("open");
					_exit(1);
				}
				dup2(fd, 1); // stdout -> file
				close(fd);
				execvp(cmd_argv[0], cmd_argv);
				perror("execvp");
				_exit(127);
			} else {
				int status;
				waitpid(pid, &status, 0);
			}
			continue;
		} else if (argc > 2 && strcmp(argv[argc - 2], "<") == 0) {
			char *cmd_argv[MAX_TOKENS];
			for (int i = 0; i < argc - 2; i++) {
				cmd_argv[i] = argv[i];
			}
			cmd_argv[argc - 2] = NULL;

			pid_t pid = fork();
			if (pid == 0) {
				int fd = open(argv[argc - 1], O_RDONLY);
				if (fd < 0) {
					perror("open");
					_exit(1);
				}
				dup2(fd, 0); // stdin <- file
				close(fd);
				execvp(cmd_argv[0], cmd_argv);
				perror("execvp");
				_exit(127);
			} else {
				int status;
				waitpid(pid, &status, 0);
			}
			continue;
		}

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

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
	char *left_argv[MAX_TOKENS], *right_argv[MAX_TOKENS];

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

		int pipe_index = -1;
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], "|") == 0) {
				pipe_index = i;
				break;
			}
		}

		if (pipe_index != -1) {
			// build left_argv and right_argv
			int left_count = 0;
			for (int i = 0; i < pipe_index; i++) {
				left_argv[left_count++] = argv[i];
			}
			left_argv[left_count] = NULL;

			int right_count = 0;
			for (int i = pipe_index + 1; i < argc; i++) {
				right_argv[right_count++] = argv[i];
			}
			right_argv[right_count] = NULL;

			int pipefd[2];
			if (pipe(pipefd) == -1) {
				perror("pipe");
				continue;
			}

			pid_t pid1 = fork();
			if (pid1 == -1) {
				perror("fork");
				close(pipefd[0]);
				close(pipefd[1]);
				continue;
			}
			if (pid1 == 0) {
				if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
					perror("dup2");
					_exit(1);
				}
				close(pipefd[0]);
				close(pipefd[1]);

				execvp(left_argv[0], left_argv);
				perror("execvp left-pipe");
				_exit(127);
			}

			pid_t pid2 = fork();
			if (pid2 == -1) {
				perror("fork");
				// careful: we already have pid1 running
				waitpid(pid1, NULL, 0);
				close(pipefd[0]);
				close(pipefd[1]);
				continue;
			}
			if (pid2 == 0) {

				if (dup2(pipefd[0], STDIN_FILENO) == -1) {
					perror("dup2");
					_exit(1);
				}
				close(pipefd[0]);
				close(pipefd[1]);

				execvp(right_argv[0], right_argv);
				perror("execvp right-pipe");
				_exit(127);
			}

			close(pipefd[0]);
			close(pipefd[1]);

			int status;
			waitpid(pid1, &status, 0);
			waitpid(pid2, &status, 0);

			continue;
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

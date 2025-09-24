#include "run.h"
#include "internal.h"
#include "str.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void handle_command(char *cmd, char cwd[]) {
  SplitResult pipe_split = split_command(cmd, "|", 128);
  if (pipe_split.count > 1) {
    int prev_fd = STDIN_FILENO;
    for (int i = 0; i < pipe_split.count; i++) {
      int pipefd[2] = {-1, -1};
      if (i != pipe_split.count - 1) {
        if (pipe(pipefd) == -1) {
          perror("pipe");
          exit(1);
        }
      }

      pid_t pid = fork();
      if (pid == 0) {
        if (prev_fd != STDIN_FILENO) {
          dup2(prev_fd, STDIN_FILENO);
          close(prev_fd);
        }
        if (i != pipe_split.count - 1) {
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[0]);
          close(pipefd[1]);
        }

        SplitResult in_split = split_command(pipe_split.tokens[i], "<", 128);
        if (in_split.count == 2) {
          int fd = open(in_split.tokens[1], O_RDONLY);
          if (fd < 0) {
            perror("open");
            exit(1);
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
          run(in_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
        } else {
          if (i == pipe_split.count - 1) {
            SplitResult append_split =
                split_command(pipe_split.tokens[i], ">>", 128);
            SplitResult out_split =
                split_command(pipe_split.tokens[i], ">", 128);
            if (append_split.count == 2) {
              int fd = open(append_split.tokens[1],
                            O_WRONLY | O_CREAT | O_APPEND, 0644);
              if (fd > 0)
                dup2(fd, STDOUT_FILENO);
              close(fd);
              run(append_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
            } else if (out_split.count == 2) {
              int fd =
                  open(out_split.tokens[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
              if (fd > 0)
                dup2(fd, STDOUT_FILENO);
              close(fd);
              run(out_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
            } else {
              run(pipe_split.tokens[i], STDIN_FILENO, STDOUT_FILENO, cwd);
            }
            free(append_split.tokens);
            free(out_split.tokens);
          } else {
            run(pipe_split.tokens[i], STDIN_FILENO, STDOUT_FILENO, cwd);
          }
        }
        free(in_split.tokens);
        exit(0);
      }

      if (prev_fd != STDIN_FILENO)
        close(prev_fd);
      if (i != pipe_split.count - 1) {
        close(pipefd[1]);
        prev_fd = pipefd[0];
      }
    }

    for (int i = 0; i < pipe_split.count; i++)
      wait(NULL);

    free(pipe_split.tokens);
    return;
  }
  free(pipe_split.tokens);

  SplitResult append_split = split_command(cmd, ">>", 128);
  SplitResult out_split = split_command(cmd, ">", 128);
  SplitResult in_split = split_command(cmd, "<", 128);

  if (append_split.count == 2) {
    int fd = open(append_split.tokens[1], O_WRONLY | O_CREAT | O_APPEND, 0644);
    run(append_split.tokens[0], STDIN_FILENO, fd, cwd);
    close(fd);
  } else if (out_split.count == 2) {
    int fd = open(out_split.tokens[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    run(out_split.tokens[0], STDIN_FILENO, fd, cwd);
    close(fd);
  } else if (in_split.count == 2) {
    int fd = open(in_split.tokens[1], O_RDONLY);
    run(in_split.tokens[0], fd, STDOUT_FILENO, cwd);
    close(fd);
  } else {
    run(cmd, STDIN_FILENO, STDOUT_FILENO, cwd);
  }

  free(append_split.tokens);
  free(out_split.tokens);
  free(in_split.tokens);
}

void run(char *cmd, int stdin_fd, int stdout_fd, char *cwd) {
  to_lowercase(cmd, ' ');

  char copy_noproc_internal[strlen(cmd) + 1];
  strcpy(copy_noproc_internal, cmd);

  if (handle_internal(copy_noproc_internal, 0))
    return;

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    if (cwd != NULL) {
      if (chdir(cwd) != 0) {
        perror("chdir");
        exit(1);
      }
    }

    if (stdin_fd != STDIN_FILENO && stdin_fd > 0) {
      dup2(stdin_fd, STDIN_FILENO);
      close(stdin_fd);
    }
    if (stdout_fd != STDOUT_FILENO && stdout_fd > 0) {
      dup2(stdout_fd, STDOUT_FILENO);
      close(stdout_fd);
    }

    char copy[strlen(cmd) + 1];
    strcpy(copy, cmd);

    char path_copy[strlen(cmd) + 1];
    strcpy(path_copy, cmd);

    char **argv = split_command(copy, " ", 128).tokens;

    if (handle_internal(cmd, 1))
      free(argv);
    else if (handle_path(path_copy))
      free(argv);
    else {
      execvp(argv[0], argv);
      free(argv);
      perror("Error");
    }

    exit(1);
  } else if (pid < 0) {
    perror("fork");
    exit(1);
  }

  waitpid(pid, NULL, 0);
}
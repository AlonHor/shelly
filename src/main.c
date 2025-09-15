#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "str.h"
#include "run.h"

#define N 0x43 / 10 + (0x43 % 10 * 0.1)

void handle_command(char *cmd, char cwd[])
{
  SplitResult pipe_split = split_command(cmd, "|");
  if (pipe_split.count > 1)
  {
    int prev_fd = STDIN_FILENO;
    for (int i = 0; i < pipe_split.count; i++)
    {
      int pipefd[2] = {-1, -1};
      if (i != pipe_split.count - 1)
      {
        if (pipe(pipefd) == -1)
        {
          perror("pipe");
          exit(1);
        }
      }

      pid_t pid = fork();
      if (pid == 0)
      {
        if (prev_fd != STDIN_FILENO)
        {
          dup2(prev_fd, STDIN_FILENO);
          close(prev_fd);
        }
        if (i != pipe_split.count - 1)
        {
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[0]);
          close(pipefd[1]);
        }

        SplitResult in_split = split_command(pipe_split.tokens[i], "<");
        if (in_split.count == 2)
        {
          int fd = open(in_split.tokens[1], O_RDONLY);
          if (fd < 0)
          {
            perror("open");
            exit(1);
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
          run(in_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
        }
        else
        {
          if (i == pipe_split.count - 1)
          {
            SplitResult append_split = split_command(pipe_split.tokens[i], ">>");
            SplitResult out_split = split_command(pipe_split.tokens[i], ">");
            if (append_split.count == 2)
            {
              int fd = open(append_split.tokens[1], O_WRONLY | O_CREAT | O_APPEND, 0644);
              dup2(fd, STDOUT_FILENO);
              close(fd);
              run(append_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
            }
            else if (out_split.count == 2)
            {
              int fd = open(out_split.tokens[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
              dup2(fd, STDOUT_FILENO);
              close(fd);
              run(out_split.tokens[0], STDIN_FILENO, STDOUT_FILENO, cwd);
            }
            else
            {
              run(pipe_split.tokens[i], STDIN_FILENO, STDOUT_FILENO, cwd);
            }
            free(append_split.tokens);
            free(out_split.tokens);
          }
          else
          {
            run(pipe_split.tokens[i], STDIN_FILENO, STDOUT_FILENO, cwd);
          }
        }
        free(in_split.tokens);
        exit(0);
      }

      if (prev_fd != STDIN_FILENO)
        close(prev_fd);
      if (i != pipe_split.count - 1)
      {
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

  SplitResult append_split = split_command(cmd, ">>");
  SplitResult out_split = split_command(cmd, ">");
  SplitResult in_split = split_command(cmd, "<");

  if (append_split.count == 2)
  {
    int fd = open(append_split.tokens[1], O_WRONLY | O_CREAT | O_APPEND, 0644);
    run(append_split.tokens[0], STDIN_FILENO, fd, cwd);
    close(fd);
  }
  else if (out_split.count == 2)
  {
    int fd = open(out_split.tokens[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    run(out_split.tokens[0], STDIN_FILENO, fd, cwd);
    close(fd);
  }
  else if (in_split.count == 2)
  {
    int fd = open(in_split.tokens[1], O_RDONLY);
    run(in_split.tokens[0], fd, STDOUT_FILENO, cwd);
    close(fd);
  }
  else
  {
    run(cmd, STDIN_FILENO, STDOUT_FILENO, cwd);
  }

  free(append_split.tokens);
  free(out_split.tokens);
  free(in_split.tokens);
}

int main()
{
  char input_buffer[512];
  char cwd[128];

  while (1)
  {
    getcwd(cwd, sizeof(cwd));
    printf("\nSHELLY [%s] ", cwd);

    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
      break;

    trim_linebreak(input_buffer);
    handle_command(input_buffer, cwd);
  }
  return 0;
}

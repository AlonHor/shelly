#include "run.h"
#include "str.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

int matches_internal(char *a, char *b)
{
  char *cmd_cpy = strdup(b);
  if (strncmp(a, split_command(cmd_cpy, " ").tokens[0], strlen(a)) == 0)
  {
    free(cmd_cpy);
    return 1;
  }
  free(cmd_cpy);
  return 0;
}

void run(char *cmd, int stdin_fd, int stdout_fd, char *cwd)
{
  to_lowercase(cmd, ' ');

  if (matches_internal("cd", cmd))
  {
    chdir(cmd + sizeof(char) * 3);
    return;
  }

  printf("gonna run %s\n", cmd);
  pid_t pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(1);
  }

  if (pid == 0)
  {
    if (cwd != NULL)
    {
      if (chdir(cwd) != 0)
      {
        perror("chdir");
        exit(1);
      }
    }

    if (stdin_fd != STDIN_FILENO)
    {
      dup2(stdin_fd, STDIN_FILENO);
      close(stdin_fd);
    }
    if (stdout_fd != STDOUT_FILENO)
    {
      dup2(stdout_fd, STDOUT_FILENO);
      close(stdout_fd);
    }

    char *copy = strdup(cmd);
    char **argv = split_command(copy, " ").tokens;

    execvp(argv[0], argv);
    perror("execvp");
    exit(1);
  }
  else if (pid < 0)
  {
    perror("fork");
    exit(1);
  }

  waitpid(pid, NULL, 0);
}
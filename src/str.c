#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void to_lowercase(char str[], char stop)
{
  while (*str != stop && *str != '\0')
  {
    if (*str >= 'A' && *str <= 'Z')
      *str += ' ';

    str++;
  }
}

void trim_linebreak(char str[])
{
  while (*str != '\0')
  {
    if (*str == '\n')
    {
      *str = '\0';
      break;
    }

    str++;
  }
}

SplitResult split_command(char cmd[], const char delim[])
{
  int bufsize = 4;
  char **argv = malloc(bufsize * sizeof(char *));
  int argc = 0;
  char *token = strtok(cmd, delim);

  while (token != NULL)
  {
    while (*token == ' ')
      token++;

    argv[argc++] = token;
    if (argc >= bufsize)
    {
      bufsize *= 2;
      argv = realloc(argv, bufsize * sizeof(char *));
    }
    token = strtok(NULL, delim);
  }
  argv[argc] = NULL;

  SplitResult split = {argv, argc};
  return split;
}
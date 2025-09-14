#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void to_lowercase(char str[], char stop)
{
  char *pCh = &str[0];
  while (*pCh != stop)
  {
    if (*pCh >= 'A' && *pCh <= 'Z')
      *pCh += ' ';

    pCh += sizeof(char);
  }
}

void trim_linebreak(char str[])
{
  char *pCh = &str[0];
  while (*pCh != '\0')
  {
    if (*pCh == '\n')
    {
      *pCh = '\0';
      break;
    }

    pCh += sizeof(char);
  }
}

struct SplitResult split_command(char cmd[], const char delim[])
{
  int bufsize = 4;
  char **argv = malloc(bufsize * sizeof(char *));
  int argc = 0;
  char *token = strtok(cmd, delim);

  while (token != NULL)
  {
    argv[argc++] = token;
    if (argc >= bufsize)
    {
      bufsize *= 2;
      argv = realloc(argv, bufsize * sizeof(char *));
    }
    token = strtok(NULL, delim);
  }
  argv[argc] = NULL;

  struct SplitResult split = {argv, argc};
  return split;
}
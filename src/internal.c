#include "str.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void cd(char *args[])
{
  chdir(args[0]);
}

void foo(char *args[])
{
  (void)args;
  printf("bar!\n");
}

typedef void (*func_t)(char **);

struct FuncMap
{
  const char *cmd;
  func_t func;
};

struct FuncMap internal_proc_functions[] = {
    {"foo", foo},
    {NULL, NULL}};

struct FuncMap internal_noproc_functions[] = {
    {"cd", cd},
    {NULL, NULL}};

func_t get_internal(const char *cmd, int proc)
{
  if (proc)
  {
    for (int i = 0; internal_proc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_proc_functions[i].cmd) == 0)
        return internal_proc_functions[i].func;
  }
  else
  {
    for (int i = 0; internal_noproc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_noproc_functions[i].cmd) == 0)
        return internal_noproc_functions[i].func;
  }
  return NULL;
}

int handle_internal(char *full_command, int proc)
{
  SplitResult full_command_split = split_command(full_command, " ");

  char *command_name = full_command_split.tokens[0];
  char *args_str = command_name + strlen(command_name) + 1;
  char *args_str_copy = strdup(args_str);

  SplitResult args_split = split_command(args_str_copy, " ");

  func_t internal = get_internal(command_name, proc);
  int found_internal = 0;
  if (internal != NULL)
  {
    internal(args_split.tokens);
    found_internal = 1;
  }

  free(full_command_split.tokens);
  free(args_str_copy);
  free(args_split.tokens);

  return found_internal;
}